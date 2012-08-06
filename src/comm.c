/* ************************************************************************
*		File: comm.c                                        Part of CircleMUD *
*	 Usage: Communication, socket handling, main(), central game loop       *
*																																					*
*	 All rights reserved.  See license.doc for complete information.        *
*																																					*
*	 Copyright (C) 1993, 94 by the Trustees of the Johns Hopkins University *
*	 CircleMUD is based on DikuMUD, Copyright (C) 1990, 1991.               *
************************************************************************ */
/* $Id: comm.c,v 1.67 2004/05/07 18:03:36 cheron Exp $ */

#define	__COMM_C__

#include "conf.h"
#include "sysdep.h"

#if CIRCLE_GNU_LIBC_MEMORY_TRACK
# include <mcheck.h>
#endif

#ifdef CIRCLE_MACINTOSH                /* Includes for the Macintosh */
#	define SIGPIPE 13
#	define SIGALRM 14
	/* GUSI headers */
#	include <sys/ioctl.h>
	/* Codewarrior dependant */
#	include <SIOUX.h>
#	include <console.h>
#endif

#ifdef CIRCLE_WINDOWS                /* Includes for Win32 */
#	ifdef __BORLANDC__
#	 include <dir.h>
#	else /* MSVC */
#	 include <direct.h>
#	endif
#	include <mmsystem.h>
#endif /* CIRCLE_WINDOWS */

#ifdef CIRCLE_AMIGA                /* Includes for the Amiga */
#	include <sys/ioctl.h>
#	include <clib/socket_protos.h>
#endif /* CIRCLE_AMIGA */

#ifdef CIRCLE_ACORN                /* Includes for the Acorn (RiscOS) */
#	include <socklib.h>
#	include <inetlib.h>
#	include <sys/ioctl.h>
#endif

/*
 * Note, most includes for all platforms are in sysdep.h.  The list of
 * files that is included is controlled by conf.h for that platform.
 */

#include "structs.h"
#include "buffer.h"
#include "utils.h"
#include "comm.h"
#include "interpreter.h"
#include "handler.h"
#include "db.h"
#include "house.h"
#include "genolc.h"
#include "oasis.h"
#include "dg_scripts.h"
#include "screen.h"
#include "events.h"
#include "color.h"

#ifdef HAVE_ARPA_TELNET_H
#include <arpa/telnet.h>
#else
#include "telnet.h"
#endif

#ifndef	INVALID_SOCKET
#define	INVALID_SOCKET (-1)
#endif

/* externs */
extern struct ban_list_element *ban_list;
extern int num_invalid;
extern char *GREETINGS;
extern const char *circlemud_version;
extern int circle_restrict;
extern int mini_mud;
extern int zone_reset;
extern int no_rent_check;
extern int bautosave;
extern ush_int DFLT_PORT;
extern const char *DFLT_DIR;
extern const char *DFLT_IP;
extern const char *LOGNAME;
extern int max_playing;
extern int nameserver_is_slow;        /* see config.c */
extern int auto_save;                /* see config.c */
extern int autosave_time;        /* see config.c */
extern int bautosave_time;        /* see config.c */
extern int use_void_disconnect;		/* see config.c */
extern ush_int buffer_opt;
unsigned long pulse = 0;	/* number of pulses since game started */
extern struct time_info_data time_info;                /* In db.c */
extern char *help;
extern int in_copyover;				/* in db.c */
extern int boot_high;
extern int total_hotboots;
extern time_t boot_time;
extern time_t copyover_time;
extern time_t boot_high_time;

/* local globals */
struct descriptor_data *descriptor_list = NULL;                /* master desc list */
int	buf_largecount = 0;                /* # of large buffers which exist */
int	buf_overflows = 0;                /* # of overflows of output */
int	buf_switches = 0;                /* # of switches from small to large buf */
int	circle_shutdown = 0;        /* clean shutdown */
int	circle_reboot = 0;                /* reboot the game after a shutdown */
int	no_specials = 0;                /* Suppress ass. of special routines */
int	max_players = 0;                /* max descriptors available */
int	tics = 0;                        /* for extern checkpointing */
int	scheck = 0;                        /* for syntax checking mode */
struct timeval null_time;        /* zero-valued time structure */
FILE *logfile = NULL;                /* Where to send the log messages. */
byte reread_wizlist;		/* signal: SIGUSR1 */
byte emergency_unban;		/* signal: SIGUSR2 */
int	dg_act_check;                /* toggle for act_trigger */
unsigned long dg_global_pulse = 0; /* number of pulses since game start */
bool fCopyOver;          /* Are we booting in copyover mode? */
ush_int	port;
socket_t mother_desc;
const char *lib_dir;
int db_command_line = 0;
static int last_desc = 0;        /* last descriptor number */

char *mysql_host;
char *mysql_db;
char *mysql_usr;
char *mysql_pwd;
unsigned int mysqlport;
char *mysqlsock;

extern int in_copyover;

/* functions in this file */
RETSIGTYPE reread_wizlists(int sig);
RETSIGTYPE unrestrict_game(int sig);
RETSIGTYPE reap(int sig);
RETSIGTYPE checkpointing(int sig);
RETSIGTYPE hupsig(int sig);
ssize_t	perform_socket_read(socket_t desc, char *read_point,size_t space_left);
ssize_t	perform_socket_write(socket_t desc, const char *txt,size_t length);
void echo_off(struct descriptor_data *d);
void echo_on(struct descriptor_data *d);
void sanity_check(void);
void circle_sleep(struct timeval *timeout);
int	get_from_q(struct txt_q *queue, char *dest, int *aliased);
void init_game(ush_int port);
void signal_setup(void);
void game_loop(socket_t mother_desc);
socket_t init_socket(ush_int port);
int	new_descriptor(socket_t s); 
int	get_max_players(void);
int	process_output(struct descriptor_data *t);
int	process_input(struct descriptor_data *t);
void timediff(struct timeval *diff, struct timeval *a, struct timeval *b);
void timeadd(struct timeval *sum, struct timeval *a, struct timeval *b);
void flush_queues(struct descriptor_data *d);
void nonblock(socket_t s);
int	perform_subst(struct descriptor_data *t, char *orig, char *subst);
int	perform_alias(struct descriptor_data *d, char *orig);
void record_usage(void);
char *make_prompt(struct descriptor_data *point);
void check_idle_passwords(void);
void heartbeat(int pulse);
struct in_addr *get_bind_addr(void);
int	parse_ip(const char *addr, struct in_addr *inaddr);
int	set_sendbuf(socket_t s);
void setup_log(const char *filename, int fd);
int	open_logfile(const char *filename, FILE *stderr_fp);
void init_descriptor (struct descriptor_data *newd, int desc);
#if	defined(POSIX)
sigfunc	*my_signal(int signo, sigfunc *func);
#endif
int system_limits(void);
void process_events(void);
void copyover_recover(void);
char *percent_color(struct descriptor_data *d, int percent);
void copyover_reinit(void);

/* extern fcnts */
void free_editor(struct descriptor_data *d);
void reboot_wizlists(void);
void boot_world(void);
void affect_update(void);        /* In spells.c */
void mobile_activity(void);
void perform_violence(void);
void show_string(struct descriptor_data *d, char *input);
int	isbanned(char *hostname);
void weather_and_time(int mode);
void ispell_init(void);
void ispell_done(void);
void write_mud_date_to_file(void);
void make_who2html(void);
void send_tip_message(void);
void load_config(void);
void load_sql_settings(void);
int	enter_player_game(struct descriptor_data *d);
void initiate_copyover(bool panic);
int copyover_check(void);
void save_all_guilds(void);
void clear_free_list(void);
void free_messages(void);
void Board_clear_all(void);
void free_social_messages(void);
void Free_Invalid_List(void);
void save_commands(void);
void free_command_list(void);


#ifdef __CXREF__
#undef FD_ZERO
#undef FD_SET
#undef FD_ISSET
#undef FD_CLR
#define	FD_ZERO(x)
#define	FD_SET(x, y) 0
#define	FD_ISSET(x, y) 0
#define	FD_CLR(x, y)
#endif


/***********************************************************************
*	 main game loop and related stuff                                    *
***********************************************************************/

#if	defined(CIRCLE_WINDOWS) || defined(CIRCLE_MACINTOSH)

/*
 * Windows doesn't have gettimeofday, so we'll simulate it.
 * The Mac doesn't have gettimeofday either.
 * Borland C++ warns: "Undefined structure 'timezone'"
 */
void gettimeofday(struct timeval *t, struct timezone *dummy)
{
#if	defined(CIRCLE_WINDOWS)
	DWORD millisec = GetTickCount();
#elif	defined(CIRCLE_MACINTOSH)
	unsigned long int millisec;
	millisec = (int)((float)TickCount() * 1000.0 / 60.0);
#endif

	t->tv_sec = (int) (millisec / 1000);
	t->tv_usec = (millisec % 1000) * 1000;
}

#endif				/* CIRCLE_WINDOWS || CIRCLE_MACINTOSH */


#define	plant_magic(x)        do { (x)[sizeof(x) - 1] = MAGIC_NUMBER; } while (0)
#define	test_magic(x)        ((x)[sizeof(x) - 1])

/* 
 * Setup system limits, such as coredumpsize on UNIX. 
 * 0 on success, -1 on error.  (From DAK)
 */ 
int system_limits(void) 
{ 
	struct rlimit rl; 

	if (getrlimit(RLIMIT_CORE, &rl) == -1) { 
		perror("get(RLIMIT_CORE)"); 
		return (-1); 
	} 

	rl.rlim_cur = rl.rlim_max; 

	if (setrlimit(RLIMIT_CORE, &rl) == -1) { 
		perror("set(RLIMIT_CORE)"); 
		return (-1); 
	} 
	return (0); 
}

int	main(int argc, char **argv)
{
	int pos = 1;

#if CIRCLE_GNU_LIBC_MEMORY_TRACK
	mtrace();	/* This must come before any use of malloc(). */
#endif

	/* Initialize these to check for overruns later. */
	plant_magic(buf);
	plant_magic(buf1);
	plant_magic(buf2);
	plant_magic(arg);

#ifdef CIRCLE_MACINTOSH
	/*
	 * ccommand() calls the command line/io redirection dialog box from
	 * Codewarriors's SIOUX library
	 */
	argc = ccommand(&argv);
	/* Initialize the GUSI library calls.  */
	GUSIDefaultSetup();
#endif

	lib_dir = DFLT_DIR;

	while ((pos < argc) && (*(argv[pos]) == '-')) {
		switch (*(argv[pos] + 1)) {
		case 'v':
			if (*(argv[pos] + 2))
				buffer_opt = atoi(argv[pos] + 2);
			else if (++pos < argc)
				buffer_opt = atoi(argv[pos]);
			else {
				mlog("SYSERR: Number expected after option -v.");
				exit(1);
			}
			break;
		case 'o':
			if (*(argv[pos] + 2))
				LOGNAME = argv[pos] + 2;
			else if (++pos < argc)
				LOGNAME = argv[pos];
			else {
				puts("SYSERR: File name to log to expected after option -o.");
				exit(1);
			}
			break;
		 case 'C': /* -C<socket number> - recover from copyover, this is the control socket */
			fCopyOver = TRUE;
			mother_desc = atoi(argv[pos]+2);
			break;
		 case 'd':
			if (*(argv[pos] + 2))
				lib_dir = argv[pos] + 2;
			else if (++pos < argc)
				lib_dir = argv[pos];
			else {
				puts("SYSERR: Directory arg expected after option -d.");
				exit(1);
			}
			break;
		 case 'D':
			if (*(argv[pos] + 2))
				mysql_db = argv[pos] + 2;
			else if (++pos < argc)
				mysql_db = argv[pos];
			else {
				puts("SYSERR: Database arg expected after option -D.");
				exit(1);
			}
			db_command_line = 1;
			break;
		case 'm':
			mini_mud = 1;
			no_rent_check = 1;
			puts("Running in minimized mode & with no rent check.");
			break;
		case 'c':
			scheck = 1;
			puts("Syntax check mode enabled.");
			break;
		case 'b':
			bautosave = 1;
			zone_reset = 0;
			puts("Builder port zone autosave enabled.");
			break;
		case 'q':
			no_rent_check = 1;
			puts("Quick boot mode -- rent check supressed.");
			break;
		case 'r':
			circle_restrict = 1;
			puts("Restricting game -- no new players allowed.");
			break;
		case 's':
			no_specials = 1;
			puts("Suppressing assignment of special routines.");
			break;
		case 'h':
			/* From: Anil Mahajan <amahajan@proxicom.com> */
			printf("Usage: %s [-b] [-c] [-m] [-q] [-r] [-s] [-d pathname] [-D database] [-v val] [port #]\n"
							"  -b             Enable builder port autosave of zones.\n"
							"  -c             Enable syntax check mode.\n"
							"  -d <directory> Specify library directory (defaults to 'lib').\n"
							"  -D <database>  MySQL database name (defaults to config file).\n"
							"  -h             Print this command line argument help.\n"
							"  -m             Start in mini-MUD mode.\n"
							"  -o <file>      Write log to <file> instead of stderr.\n"
							"  -q             Quick boot (doesn't scan rent for object limits)\n"
							"  -r             Restrict MUD -- no new players allowed.\n"
							"  -s             Suppress special procedure assignments.\n",
								 argv[0]
			);
			exit(0);
		default:
			printf("SYSERR: Unknown option -%c in argument string.\n", *(argv[pos] + 1));
			break;
		}
		pos++;
	}

	if (pos < argc) {
		if (!isdigit(*argv[pos])) {
			printf("Usage: %s [-b] [-c] [-m] [-q] [-r] [-s] [-d pathname] [-D database] [port #]\n", argv[0]);
			exit(1);
		} else if ((port = atoi(argv[pos])) <= 1024) {
			printf("SYSERR: Illegal port number %d.\n", port);
			exit(1);
		}
	}

	/* All arguments have been parsed, try to open log file. */
	setup_log(LOGNAME, STDERR_FILENO);

	/*
	 * Moved here to distinguish command line options and to show up
	 * in the log if stderr is redirected to a file.
	 */
	mlog("%s", circlemud_version);
	mlog(DG_SCRIPT_VERSION);

	init_buffers();

	if (chdir(lib_dir) < 0) {
		perror("SYSERR: Fatal error changing to data directory");
		exit(1);
	}

	/*
	 * Loading copyover settings from copyover file
	 * Torgny Bjers <artovil@arcanerealms.org>, 2002-06-19
	 */
	if (fCopyOver)
		copyover_reinit();

	mlog("Using %s as data directory.", lib_dir);

  /* Cheron: Set core size for core dumps */
  if (system_limits()) {
    sprintf(buf, "SYSERR: Error setting core limit '%m'");
    mlog(buf);
  } else
    mlog("Core size set.");

	mlog("Loading database settings.");
	load_sql_settings();
	init_mysql(mysql_db);

	if (scheck)
		boot_world();
	else {
		mlog("Running game on port %d.", port);
		init_game(port);
	}

	if (circle_reboot < 3) {
		mlog("Clearing game world.");
		destroy_db();
		
		if (!scheck) {
			mlog("Clearing other memory.");
			free_player_index();		/* db.c */
			free_messages();				/* fight.c */
			clear_free_list();			/* mail.c */
			free_text_files();			/* db.c */
			Board_clear_all();			/* boards.c */
			free_social_messages();	/* act.social.c */
			Free_Invalid_List();		/* ban.c */
			free_command_list();		/* commands.c */
		}
	}
	
	mlog("Done.");

	exit_buffers();
	
	return (0);
}


/*
 * Read system settings from the copyover file since they
 * do not initiate properly when sent through the command
 * line with execl. (Perhaps the string should be NULL
 * terminated, but hell, this is better anyway.
 * Torgny Bjers <artovil@arcanerealms.org>, 2002-06-19
 */
void copyover_reinit(void)
{
	FILE *fl;
	int line_num = 0, tmp, tmp1, tmp2, tmp3;
	time_t ttmp, ttmp1;
	char readbuf[256];

	mlog("Copyover system initialization initiated");

	fl = fopen (COPYOVER_FILE, "r");

	if (!fl) { /* there are some descriptors open which will hang forever then ? */
		perror ("copyover_recover:fopen");
		mlog("Copyover file not found. Exiting.\r\n");
		exit (1);
	}

	/* Read the first line, port number */
	line_num += get_line(fl, readbuf);

	if (sscanf(readbuf, "#%d", &tmp) != 1) {
		mlog("SYSERR: Format error in copyover data line %d (%s:%d)", line_num, __FUNCTION__, __LINE__);
		exit(1);
	}

	port = tmp;

	/* Read the second line, settings */
	line_num += get_line(fl, readbuf);

	if (sscanf(readbuf, "%d %d %d %d", &tmp, &tmp1, &tmp2, &tmp3) != 4) {
		mlog("SYSERR: Format error in copyover data line %d (%s:%d)", line_num, __FUNCTION__, __LINE__);
		exit(1);
	}
	
	no_specials = tmp;
	mini_mud = tmp1;
	no_rent_check = tmp2;
	bautosave = tmp3;

	/* Read the third line, times and maxon */
	line_num += get_line(fl, readbuf);

	if (sscanf(readbuf, "%lu %lu %d %d", &ttmp, &ttmp1, &tmp2, &tmp3) != 4) {
		mlog("SYSERR: Format error in copyover data line %d (%s:%d)", line_num, __FUNCTION__, __LINE__);
		exit(1);
	}
	
	boot_time = ttmp;
	boot_high_time = ttmp1;
	boot_high = tmp2;
	total_hotboots = tmp3;

	/* Last, read the strings, lib_dir and mysql_db */
	lib_dir = fread_string(fl, readbuf);
	mysql_db = fread_string(fl, readbuf);

	fclose (fl);
}


/* Reload players after a copyover */
void copyover_recover(void)
{
	struct descriptor_data *d;
	FILE *fp;
	char host[1024];
	int desc, player_i, line_num = 0;
	bool fOld;
	char name[MAX_INPUT_LENGTH];
	char readbuf[256];
				
	mlog("Copyover recovery initiated");
				
	fp = fopen (COPYOVER_FILE, "r");

	if (!fp) { /* there are some descriptors open which will hang forever then ? */
		perror ("copyover_recover:fopen");
		mlog("Copyover file not found. Exitting.\r\n");
		exit (1);
	}

	unlink (COPYOVER_FILE); /* In case something crashes - doesn't prevent reading        */

	line_num += get_line(fp, readbuf);
	line_num += get_line(fp, readbuf);
	line_num += get_line(fp, readbuf);
	line_num += get_line(fp, readbuf);
	line_num += get_line(fp, readbuf);

	for (;;) {
	
		fOld = TRUE;
		fscanf (fp, "%d %s %s\n", &desc, name, host);
		if (desc == -1)
		break;

		/* Write something, and check if it goes error-free */                
		if (write_to_descriptor (desc, "\r\nFolding initiated...\r\n") < 0) {
			close (desc); /* nope */
			continue;
		}

		/* create a new descriptor */
		CREATE (d, struct descriptor_data, 1);
		memset ((char *) d, 0, sizeof (struct descriptor_data));
		init_descriptor (d,desc); /* set up various stuff */

		strcpy(d->host, host);
		d->next = descriptor_list;
		descriptor_list = d;

		d->connected = CON_CLOSE;

		/* Now, find the pfile */

		CREATE(d->character, struct char_data, 1);
		clear_char(d->character);
		CREATE(d->character->player_specials, struct player_special_data, 1);
		d->character->desc = d;

		if ((player_i = load_char(name, d->character)) >= 0) {
			GET_PFILEPOS(d->character) = player_i;
			if (!PLR_FLAGGED(d->character, PLR_DELETED))
				REMOVE_BIT(PLR_FLAGS(d->character),PLR_WRITING | PLR_MAILING | PLR_CRYO);
			else
				fOld = FALSE;
		}
		else
			fOld = FALSE;

		if (!fOld) { /* Player file not found?! */
			write_to_descriptor (desc, "\n\rSomehow, your character was lost during the folding. Sorry.\r\n");
			close_socket (d);                        
		}
		else { /* ok! */
			write_to_descriptor (desc, "\r\nFolding complete.\r\n");
			enter_player_game(d);
			d->connected = CON_PLAYING;
			look_at_room(d->character, 0);
		}

	}

	total_hotboots++;

	fclose (fp);
	in_copyover = 0;
}


/* Init sockets, run game, and cleanup sockets */
void init_game(ush_int port)
{

	/* We don't want to restart if we crash before we get up. */
	touch(KILLSCRIPT_FILE);

	circle_srandom(time(0));

	load_config();

	mlog("Finding player limit.");
	max_players = get_max_players();

	if (!fCopyOver) { /* If copyover mother_desc is already set up */
		mlog("Opening mother connection.");
		mother_desc = init_socket(port);
	}

	event_init();

	boot_db();

#if	defined(CIRCLE_UNIX) || defined(CIRCLE_MACINTOSH)
	mlog("Signal trapping.");
	signal_setup();
#endif

	/* If we made it this far, we will be able to restart without problem. */
	remove(KILLSCRIPT_FILE);

	if (fCopyOver) /* reload players */
		copyover_recover();
	mlog("Entering game loop.");

	mlog("Initializing spellchecker.");
	ispell_init();

	if (!fCopyOver) {
		mlog("Resetting boot high time.");
		boot_high_time = time(0);
	}

	game_loop(mother_desc);

	ispell_done();

	if (circle_reboot < 3) {
		Crash_save_all();

		mlog("Saving player houses.");
		house_save_all();

		mlog("Saving all guilds.");
		save_all_guilds();

		mlog("Saving commands.");
		save_commands();

		mlog("Saving current MUD time.");
		write_mud_date_to_file();
	}

	mlog("Closing all sockets.");
	while (descriptor_list)
		close_socket(descriptor_list);

	CLOSE_SOCKET(mother_desc);

	if (circle_reboot < 3 && bautosave)
		save_all();

	kill_mysql();

	if (circle_reboot) {
		mlog("Rebooting.");
		exit(52);	/* what's so great about HHGTTG, anyhow? */
	}

	mlog("Normal termination of game.");
}



/*
 * init_socket sets up the mother descriptor - creates the socket, sets
 * its options up, binds it, and listens.
 */
socket_t init_socket(ush_int port)
{
	socket_t s;
	struct sockaddr_in sa;
	int opt;

#ifdef CIRCLE_WINDOWS
	{
		WORD wVersionRequested;
		WSADATA wsaData;

		wVersionRequested = MAKEWORD(1, 1);

		if (WSAStartup(wVersionRequested, &wsaData) != 0) {
			mlog("SYSERR: WinSock not available!");
			exit(1);
		}

		/*
		 * 4 = stdin, stdout, stderr, mother_desc.  Windows might
		 * keep sockets and files separate, in which case this isn't
		 * necessary, but we will err on the side of caution.
		 */
		if ((wsaData.iMaxSockets - 4) < max_players) {
			max_players = wsaData.iMaxSockets - 4;
		}
		mlog("Max players set to %d", max_players);

		if ((s = socket(PF_INET, SOCK_STREAM, 0)) == INVALID_SOCKET) {
			mlog("SYSERR: Error opening network connection: Winsock error #%d",
					WSAGetLastError());
			exit(1);
		}
	}
#else
	/*
	 * Should the first argument to socket() be AF_INET or PF_INET?  I don't
	 * know, take your pick.  PF_INET seems to be more widely adopted, and
	 * Comer (_Internetworking with TCP/IP_) even makes a point to say that
	 * people erroneously use AF_INET with socket() when they should be using
	 * PF_INET.  However, the man pages of some systems indicate that AF_INET
	 * is correct; some such as ConvexOS even say that you can use either one.
	 * All implementations I've seen define AF_INET and PF_INET to be the same
	 * number anyway, so the point is (hopefully) moot.
	 */

	if ((s = socket(PF_INET, SOCK_STREAM, 0)) < 0) {
		perror("SYSERR: Error creating socket");
		exit(1);
	}
#endif																/* CIRCLE_WINDOWS */

#if	defined(SO_REUSEADDR) && !defined(CIRCLE_MACINTOSH)
	opt = 1;
	if (setsockopt(s, SOL_SOCKET, SO_REUSEADDR, (char *) &opt, sizeof(opt)) < 0){
		perror("SYSERR: setsockopt REUSEADDR");
		exit(1);
	}
#endif

	set_sendbuf(s);

/*
 * The GUSI sockets library is derived from BSD, so it defines
 * SO_LINGER, even though setsockopt() is unimplimented.
 *        (from Dean Takemori <dean@UHHEPH.PHYS.HAWAII.EDU>)
 */
#if	defined(SO_LINGER) && !defined(CIRCLE_MACINTOSH)
	{
		struct linger ld;

		ld.l_onoff = 0;
		ld.l_linger = 0;
		if (setsockopt(s, SOL_SOCKET, SO_LINGER, (char *) &ld, sizeof(ld)) < 0)
			perror("SYSERR: setsockopt SO_LINGER");        /* Not fatal I suppose. */
	}
#endif

	/* Clear the structure */
	memset((char *)&sa, 0, sizeof(sa));

	sa.sin_family = AF_INET;
	sa.sin_port = htons(port);
	sa.sin_addr = *(get_bind_addr());

	if (bind(s, (struct sockaddr *) &sa, sizeof(sa)) < 0) {
		perror("SYSERR: bind");
		CLOSE_SOCKET(s);
		exit(1);
	}
	nonblock(s);
	listen(s, 5);
	return (s);
}


int	get_max_players(void)
{
#ifndef	CIRCLE_UNIX
	return (max_playing);
#else

	int max_descs = 0;
	const char *method;

/*
 * First, we'll try using getrlimit/setrlimit.  This will probably work
 * on most systems.  HAS_RLIMIT is defined in sysdep.h.
 */
#ifdef HAS_RLIMIT
	{
		struct rlimit limit;

		/* find the limit of file descs */
		method = "rlimit";
		if (getrlimit(RLIMIT_NOFILE, &limit) < 0) {
			perror("SYSERR: calling getrlimit");
			exit(1);
		}

		/* set the current to the maximum */
		limit.rlim_cur = limit.rlim_max;
		if (setrlimit(RLIMIT_NOFILE, &limit) < 0) {
			perror("SYSERR: calling setrlimit");
			exit(1);
		}
#ifdef RLIM_INFINITY
		if (limit.rlim_max == RLIM_INFINITY)
			max_descs = max_playing + NUM_RESERVED_DESCS;
		else
			max_descs = MIN(max_playing + NUM_RESERVED_DESCS, limit.rlim_max);
#else
		max_descs = MIN(max_playing + NUM_RESERVED_DESCS, limit.rlim_max);
#endif
	}

#elif	defined (OPEN_MAX) || defined(FOPEN_MAX)
#if	!defined(OPEN_MAX)
#define	OPEN_MAX FOPEN_MAX
#endif
	method = "OPEN_MAX";
	max_descs = OPEN_MAX;                /* Uh oh.. rlimit didn't work, but we have
																 * OPEN_MAX */
#elif	defined (_SC_OPEN_MAX)
	/*
	 * Okay, you don't have getrlimit() and you don't have OPEN_MAX.  Time to
	 * try the POSIX sysconf() function.  (See Stevens' _Advanced Programming
	 * in the UNIX Environment_).
	 */
	method = "POSIX sysconf";
	errno = 0;
	if ((max_descs = sysconf(_SC_OPEN_MAX)) < 0) {
		if (errno == 0)
			max_descs = max_playing + NUM_RESERVED_DESCS;
		else {
			perror("SYSERR: Error calling sysconf");
			exit(1);
		}
	}
#else
	/* if everything has failed, we'll just take a guess */
	method = "random guess";
	max_descs = max_playing + NUM_RESERVED_DESCS;
#endif

	/* now calculate max _players_ based on max descs */
	max_descs = MIN(max_playing, max_descs - NUM_RESERVED_DESCS);

	if (max_descs <= 0) {
		mlog("SYSERR: Non-positive max player limit!  (Set at %d using %s).",
						max_descs, method);
		exit(1);
	}
	mlog("   Setting player limit to %d using %s.", max_descs, method);
	return (max_descs);
#endif /* CIRCLE_UNIX */
}



/*
 * game_loop contains the main loop which drives the entire MUD.  It
 * cycles once every 0.10 seconds and is responsible for accepting new
 * new connections, polling existing connections for input, dequeueing
 * output and sending it out to players, and calling "heartbeat" functions
 * such as mobile_activity().
 */
void game_loop(socket_t mother_desc) 
{
	fd_set input_set, output_set, exc_set, null_set;
	struct timeval last_time, opt_time, process_time, temp_time;
	struct timeval before_sleep, now, timeout;
	char comm[MAX_INPUT_LENGTH];
	struct descriptor_data *d, *next_d;
	int missed_pulses = 0, maxdesc, aliased;

	/* initialize various time values */
	null_time.tv_sec = 0;
	null_time.tv_usec = 0;
	opt_time.tv_usec = OPT_USEC;
	opt_time.tv_sec = 0;
	FD_ZERO(&null_set);

	gettimeofday(&last_time, (struct timezone *) 0);

	/* The Main Loop.  The Big Cheese.  The Top Dog.  The Head Honcho.  The.. */
	while (!circle_shutdown) {

		/* Sleep if we don't have any connections */
		if (descriptor_list == NULL) {
			mlog("No connections.  Going to sleep.");
			make_who2html();
			if (!(pulse % PULSE_VIOLENCE))
				FD_ZERO(&input_set);
			FD_SET(mother_desc, &input_set);
			if (select(mother_desc + 1, &input_set, (fd_set *) 0, (fd_set *) 0, NULL) < 0) {
				if (errno == EINTR)
					mlog("Waking up to process signal.");
				else
					perror("SYSERR: Select coma");
			} else
				mlog("New connection.  Waking up.");
			gettimeofday(&last_time, (struct timezone *) 0);
		}
		/* Set up the input, output, and exception sets for select(). */
		FD_ZERO(&input_set);
		FD_ZERO(&output_set);
		FD_ZERO(&exc_set);
		FD_SET(mother_desc, &input_set);

		maxdesc = mother_desc;
		for (d = descriptor_list; d; d = d->next) {
#ifndef	CIRCLE_WINDOWS
			if (d->descriptor > maxdesc)
				maxdesc = d->descriptor;
#endif
			FD_SET(d->descriptor, &input_set);
			FD_SET(d->descriptor, &output_set);
			FD_SET(d->descriptor, &exc_set);
		}

		/*
		 * At this point, we have completed all input, output and heartbeat
		 * activity from the previous iteration, so we have to put ourselves
		 * to sleep until the next 0.1 second tick.  The first step is to
		 * calculate how long we took processing the previous iteration.
		 */
		
		gettimeofday(&before_sleep, (struct timezone *) 0); /* current time */
		timediff(&process_time, &before_sleep, &last_time);

		/*
		 * If we were asleep for more than one pass, count missed pulses and sleep
		 * until we're resynchronized with the next upcoming pulse.
		 */
		if (process_time.tv_sec == 0 && process_time.tv_usec < OPT_USEC) {
			missed_pulses = 0;
		} else {
			missed_pulses = process_time.tv_sec * PASSES_PER_SEC;
			missed_pulses += process_time.tv_usec / OPT_USEC;
			process_time.tv_sec = 0;
			process_time.tv_usec = process_time.tv_usec % OPT_USEC;
		}

		/* Calculate the time we should wake up */
		timediff(&temp_time, &opt_time, &process_time);
		timeadd(&last_time, &before_sleep, &temp_time);

		/* Now keep sleeping until that time has come */
		gettimeofday(&now, (struct timezone *) 0);
		timediff(&timeout, &last_time, &now);

		/* Go to sleep */
		do {
			circle_sleep(&timeout);
			gettimeofday(&now, (struct timezone *) 0);
			timediff(&timeout, &last_time, &now);
		} while (timeout.tv_usec || timeout.tv_sec);

		/* Poll (without blocking) for new input, output, and exceptions */
		if (select(maxdesc + 1, &input_set, &output_set, &exc_set, &null_time) < 0) {
			perror("SYSERR: Select poll");
			return;
		}
		/* If there are new connections waiting, accept them. */
		if (FD_ISSET(mother_desc, &input_set))
			new_descriptor(mother_desc);

		/* Kick out the freaky folks in the exception set and marked for close */
		for (d = descriptor_list; d; d = next_d) {
			next_d = d->next;
			if (FD_ISSET(d->descriptor, &exc_set)) {
				FD_CLR(d->descriptor, &input_set);
				FD_CLR(d->descriptor, &output_set);
				close_socket(d);
			}
		}

		/* Process descriptors with input pending */
		for (d = descriptor_list; d; d = next_d) {
			next_d = d->next;
			if (FD_ISSET(d->descriptor, &input_set))
				if (process_input(d) < 0)
					close_socket(d);
		}

		/* Process commands we just read from process_input */
		for (d = descriptor_list; d; d = next_d) {
			next_d = d->next;

			/*
			 * Not combined to retain --(d->wait) behavior. -gg 2/20/98
			 * If no wait state, no subtraction.  If there is a wait
			 * state then 1 is subtracted. Therefore we don't go less
			 * than 0 ever and don't require an 'if' bracket. -gg 2/27/99
			 */
			if (d->character) {
				GET_WAIT_STATE(d->character) -= (GET_WAIT_STATE(d->character) > 0);

				if (GET_WAIT_STATE(d->character))
					continue;
			}

			if (!get_from_q(&d->input, comm, &aliased))
				continue;

			if (d->character) {
				/* Reset the idle timer & pull char back from void if necessary */
				d->character->char_specials.timer = 0;
				if (use_void_disconnect && (STATE(d) == CON_PLAYING || STATE(d) == CON_COPYOVER) && GET_WAS_IN(d->character) != NOWHERE) {
					if (IN_ROOM(d->character) != NOWHERE)
						char_from_room(d->character);
					char_to_room(d->character, GET_WAS_IN(d->character));
					GET_WAS_IN(d->character) = NOWHERE;
					act("$n has returned.", TRUE, d->character, 0, 0, TO_ROOM);
				}
				GET_WAIT_STATE(d->character) = 1;
			}
			d->has_prompt = 0;

			if (d->showstr_count) /* Reading something w/ pager */
				show_string(d, comm);
			else if (EDITING(d))    /* Writing boards, mail, etc. */
				string_add(d, comm);
			else if (STATE(d) != CON_PLAYING && STATE(d) != CON_COPYOVER) /* In menus, etc. */
				nanny(d, comm);
			else {                        /* else: we're playing normally. */
				if (aliased)                /* To prevent recursive aliases. */
					d->has_prompt = 1;        /* To get newline before next cmd output. */
				else if (perform_alias(d, comm))    /* Run it through aliasing system */
					get_from_q(&d->input, comm, &aliased);
				command_interpreter(d->character, comm); /* Send it to interpreter */
			}
		}

		/* Send queued output out to the operating system (ultimately to user). */
		for (d = descriptor_list; d; d = next_d) {
			next_d = d->next;
			if (*(d->output) && FD_ISSET(d->descriptor, &output_set)) {
				/* Output for this player is ready */
				if (process_output(d) < 0)
					close_socket(d);
				else
					d->has_prompt = 1;
			}
		}

		/* Print prompts for other descriptors who had no other output */
		for (d = descriptor_list; d; d = d->next) {
			if (!d->has_prompt) {
				write_to_descriptor(d->descriptor, make_prompt(d));
				d->has_prompt = 1;
			}
		}

		/* Kick out folks in the CON_CLOSE or CON_DISCONNECT state */
		for (d = descriptor_list; d; d = next_d) {
			next_d = d->next;
			if (STATE(d) == CON_CLOSE || STATE(d) == CON_DISCONNECT)
				close_socket(d);
		}

		/*
		 * Now, we execute as many pulses as necessary--just one if we haven't
		 * missed any pulses, or make up for lost time if we missed a few
		 * pulses by sleeping for too long.
		 */
		missed_pulses++;

		if (missed_pulses <= 0) {
			mlog("SYSERR: **BAD** MISSED_PULSES NONPOSITIVE (%d), TIME GOING BACKWARDS!!", missed_pulses);
			missed_pulses = 1;
		}

		/* If we missed more than 30 seconds worth of pulses, just do 30 secs */
		if (missed_pulses > 30 RL_SEC) {
			mlog("SYSERR: Missed %d seconds worth of pulses.", missed_pulses / PASSES_PER_SEC);
			missed_pulses = 30 RL_SEC;
		}

		/* Now execute the heartbeat functions */
		while (missed_pulses--) {
			if (in_copyover && (copyover_check()))
				initiate_copyover(FALSE);
			else 
				heartbeat(++pulse);
		}

		/* Check for any signals we may have received. */
		if (reread_wizlist) {
			reread_wizlist = FALSE;
			extended_mudlog(CMP, SYSL_GENERAL, TRUE, "Signal received - rereading wizlists.");
			reboot_wizlists();
		}
		if (emergency_unban) {
			emergency_unban = FALSE;
			extended_mudlog(BRF, SYSL_GENERAL, TRUE, "Received SIGUSR2 - completely unrestricting game (emergent)");
			ban_list = NULL;
			circle_restrict = 0;
			num_invalid = 0;
		}


#ifdef CIRCLE_UNIX
		/* Update tics for deadlock protection (UNIX only) */
		tics++;
#endif
	}
}


void heartbeat(int pulse)
{
	static int mins_since_crashsave = 0;

	event_process();

	dg_global_pulse++;

	process_events();

  if (!(pulse % PULSE_DG_SCRIPT))
		script_trigger_check();

	if (!(pulse % PULSE_SANITY))
		sanity_check();

	/* Clear out all the global buffers now in case someone forgot. */
	if (!(pulse % PULSE_BUFFER))
		release_all_buffers(pulse);
	
	if (!(pulse % PULSE_ZONE)) {
		if (!bautosave)
			zone_update();
	}

	if (!(pulse % PULSE_IDLEPWD))                /* 15 seconds */
		check_idle_passwords();

	if (!(pulse % PULSE_MOBILE))
		mobile_activity();

	if (!(pulse % PULSE_VIOLENCE))
		perform_violence();

	if (!(pulse % (60 * PASSES_PER_SEC))) {
		if (!in_copyover) {
			make_who2html();
			affect_update();
			point_update();
			extended_mudlog(CMP, SYSL_TICKS, FALSE, "Calling affect/point update.");
		}
	}

	if (!(pulse % (90 * PASSES_PER_SEC))) {
		extended_mudlog(CMP, SYSL_TICKS, FALSE, "Sending tip messages.");
		send_tip_message();
	}

	if (!(pulse % (SECS_PER_MUD_HOUR * PASSES_PER_SEC))) {
		weather_and_time(1);
		condition_update();
		extended_mudlog(CMP, SYSL_TICKS, FALSE, "Calling weather and time.");
	}

	if (bautosave && !(pulse % ((bautosave_time * 60) * PASSES_PER_SEC))) {
		extended_mudlog(BRF, SYSL_TICKS, FALSE, "Auto-saving all zones.");
		save_all();
	}

	if (auto_save && !(pulse % PULSE_AUTOSAVE)) {        /* 1 minute */
		if (++mins_since_crashsave >= autosave_time) {
			mins_since_crashsave = 0;
			Crash_save_all();
			house_save_all();
			extended_mudlog(CMP, SYSL_TICKS, FALSE, "Crash-saving.");
		}
	}
	if (!(pulse % PULSE_USAGE)) {
		record_usage();
		mysql_keepalive();
	}

	if (!(pulse % PULSE_TIMESAVE))
		write_mud_date_to_file();

	/* Every pulse! Don't want them to stink the place up... */
	extract_pending_chars();
}


/* ******************************************************************
*	 general utility stuff (for local use)                            *
****************************************************************** */

/*
 *  new code to calculate time differences, which works on systems
 *  for which tv_usec is unsigned (and thus comparisons for something
 *  being < 0 fail).  Based on code submitted by ss@sirocco.cup.hp.com.
 */

/*
 * code to return the time difference between a and b (a-b).
 * always returns a nonnegative value (floors at 0).
 */
void timediff(struct timeval *rslt, struct timeval *a, struct timeval *b)
{
	if (a->tv_sec < b->tv_sec)
		*rslt = null_time;
	else if (a->tv_sec == b->tv_sec) {
		if (a->tv_usec < b->tv_usec)
			*rslt = null_time;
		else {
			rslt->tv_sec = 0;
			rslt->tv_usec = a->tv_usec - b->tv_usec;
		}
	} else {                        /* a->tv_sec > b->tv_sec */
		rslt->tv_sec = a->tv_sec - b->tv_sec;
		if (a->tv_usec < b->tv_usec) {
			rslt->tv_usec = a->tv_usec + 1000000 - b->tv_usec;
			rslt->tv_sec--;
		} else
			rslt->tv_usec = a->tv_usec - b->tv_usec;
	}
}

/*
 * Add 2 time values.
 *
 * Patch sent by "d. hall" <dhall@OOI.NET> to fix 'static' usage.
 */
void timeadd(struct timeval *rslt, struct timeval *a, struct timeval *b)
{
	rslt->tv_sec = a->tv_sec + b->tv_sec;
	rslt->tv_usec = a->tv_usec + b->tv_usec;

	while (rslt->tv_usec >= 1000000) {
		rslt->tv_usec -= 1000000;
		rslt->tv_sec++;
	}
}


void record_usage(void)
{
  time_t curtime;
	int sockets_connected = 0, sockets_playing = 0, sockets_ic = 0;
	struct descriptor_data *d;

	curtime = time (NULL);

	for (d = descriptor_list; d; d = d->next) {
		sockets_connected++;
		if ((STATE(d) == CON_PLAYING || STATE(d) == CON_COPYOVER)) {
			sockets_playing++;
			if (IS_IC(d->character))
				sockets_ic++;
		}
	}

	extended_mudlog(BRF, SYSL_TICKS, FALSE, "nusage: %d sockets connected, %d sockets playing (%d IC)",
					sockets_connected, sockets_playing, sockets_ic);

#ifdef RUSAGE_SELF
	{
		struct rusage ru;

		getrusage(RUSAGE_SELF, &ru);

		mysqlWrite("INSERT INTO %s (Sockets, Players, IC, UTCDate, UserTime, SysTime, MinFaults, MajFaults, LastReboot, LastCopyover, Copyovers) VALUES(%d, %d, %d, %ld, %ld, %ld, %ld, %ld, %ld, %ld, %d);", TABLE_USAGE_HISTORY, sockets_connected, sockets_playing, sockets_ic, curtime, ru.ru_utime.tv_sec, ru.ru_stime.tv_sec, ru.ru_minflt, ru.ru_majflt, boot_time, copyover_time, total_hotboots);

		extended_mudlog(BRF, SYSL_TICKS, FALSE, "RUSAGE: Sockets: %d, Players: %d, IC: %d, UTCDate: %ld, UserTime: %ld, SysTime: %ld, MinFaults: %ld, MajFaults: %ld, LastReboot: %ld, LastCopyover: %ld, Copyovers: %d",
						sockets_connected, sockets_playing, sockets_ic, curtime, ru.ru_utime.tv_sec, ru.ru_stime.tv_sec, ru.ru_minflt, ru.ru_majflt, boot_time, copyover_time, total_hotboots);
	}
#endif

}



/*
 * Turn off echoing (specific to telnet client)
 */
void echo_off(struct descriptor_data *d)
{
	char off_string[] =
	{
		(char) IAC,
		(char) WILL,
		(char) TELOPT_ECHO,
		(char) 0,
	};

	write_to_output(d, TRUE, "%s", off_string);
}


/*
 * Turn on echoing (specific to telnet client)
 */
void echo_on(struct descriptor_data *d)
{
	char on_string[] =
	{
		(char) IAC,
		(char) WONT,
		(char) TELOPT_ECHO,
		(char) 0
	};

	write_to_output(d, TRUE, "%s", on_string);
}


char *percent_color(struct descriptor_data *d, int percent)
{
	if (percent <= 33)
		return(CCRED(d->character, C_NRM));
	else if (percent > 33 && percent <= 66)
		return(CCCYN(d->character, C_NRM));
	else if (percent > 66)
		return(CCGRN(d->character, C_NRM));
	else
		return(CCNRM(d->character, C_NRM));
}


char *make_prompt(struct descriptor_data *d)
{
	static char prompt[MAX_PROMPT_LENGTH];
	int percent = 0;

	/* Note, prompt is truncated at MAX_PROMPT_LENGTH chars (structs.h) */

	if (d->showstr_count) {
		snprintf(prompt, sizeof(prompt),
				"[ Return to continue, [Q]uit, [R]efresh, [B]ack, or page number (%d/%d) ]\r\n",
				d->showstr_page, d->showstr_count);
	} else if (EDITING(d))
		strcpy(prompt, "] ");	/* strcpy: OK (for 'MAX_PROMPT_LENGTH >= 3') */
	else if ((STATE(d) == CON_PLAYING || STATE(d) == CON_COPYOVER) && !IS_NPC(d->character) && PRF_FLAGGED(d->character, PRF_HASPROMPT)) {
		int count;
		size_t len = 0;
		
		*prompt = '\0';

		if (len < sizeof(prompt)) {
			count = snprintf(prompt + len, sizeof(prompt) - len, "[%s%s%s] ", CCCYN(d->character, C_NRM), (IS_IC(d->character)?"IC":"OOC"), CCNRM(d->character, C_SPR));
			if (count >= 0)
				len += count;
		}

		if (GET_INVIS_LEV(d->character) != RIGHTS_NONE && len < sizeof(prompt)) {
			count = snprintf(prompt + len, sizeof(prompt) - len, "i-%s ", wiz_char_rights(GET_INVIS_LEV(d->character)));
			if (count >= 0)
				len += count;
		}
	
		if (GET_MAX_HIT(d->character) != 0)
			percent = (100 * GET_HIT(d->character)) / GET_MAX_HIT(d->character);
		else
			percent = 0;

		if (percent < 0)
			percent = 0;

		if (PRF_FLAGGED(d->character, PRF_DISPHP) && len < sizeof(prompt)) {
			count = snprintf(prompt + len, sizeof(prompt) - len, "%d%sH%s ", GET_HIT(d->character), percent_color(d, percent), CCNRM(d->character, C_SPR));
			if (count >= 0)
				len += count;
		}

		if (GET_MAX_MANA(d->character) != 0)
			percent = (100 * GET_MANA(d->character)) / GET_MAX_MANA(d->character);
		else
			percent = 0;

		if (percent < 0)
			percent = 0;

		if (PRF_FLAGGED(d->character, PRF_DISPMANA) && len < sizeof(prompt)) {
			count = snprintf(prompt + len, sizeof(prompt) - len, "%d%sM%s ", GET_MANA(d->character), percent_color(d, percent), CCNRM(d->character, C_SPR));
			if (count >= 0)
				len += count;
		}

		if (GET_MAX_MOVE(d->character) != 0)
			percent = (100 * GET_MOVE(d->character)) / GET_MAX_MOVE(d->character);
		else
			percent = 0;

		if (percent < 0)
			percent = 0;

		if (PRF_FLAGGED(d->character, PRF_DISPMOVE) && len < sizeof(prompt)) {
			count = snprintf(prompt + len, sizeof(prompt) - len, "%d%sV%s ", GET_MOVE(d->character), percent_color(d, percent), CCNRM(d->character, C_SPR));
			if (count >= 0)
				len += count;
		}

		if ((SESS_FLAGGED(d->character, SESS_AFW) || SESS_FLAGGED(d->character, SESS_AFK)) && len < sizeof(prompt)) {
			count = snprintf(prompt + len, sizeof(prompt) - len, "%s ", (SESS_FLAGGED(d->character, SESS_AFW)) ? "AFW" : "AFK");
			if (count >= 0)
				len += count;
		}

		if (len < sizeof(prompt))
			strncat(prompt, "> ", sizeof(prompt) - len - 1);	/* strncat: OK */

	} else if ((STATE(d) == CON_PLAYING || STATE(d) == CON_COPYOVER) && IS_NPC(d->character))
		snprintf(prompt, sizeof(prompt), "%s> ", GET_NAME(d->character));
	else
		*prompt = '\0';

	return (prompt);
}


/*
 * NOTE: 'txt' must be at most MAX_INPUT_LENGTH big.
 */
void write_to_q(const char *txt, struct txt_q *queue, int aliased)
{
	struct txt_block *newt;

	CREATE(newt, struct txt_block, 1);
	newt->text = str_dup(txt);
	newt->aliased = aliased;

	/* queue empty? */
	if (!queue->head) {
		newt->next = NULL;
		queue->head = queue->tail = newt;
	} else {
		queue->tail->next = newt;
		queue->tail = newt;
		newt->next = NULL;
	}
}


/*
 * NOTE: 'dest' must be at least MAX_INPUT_LENGTH big.
 */
int	get_from_q(struct txt_q *queue, char *dest, int *aliased)
{
	struct txt_block *tmp;

	/* queue empty? */
	if (!queue->head)
		return (0);

	strcpy(dest, queue->head->text);	/* strcpy: OK (mutual MAX_INPUT_LENGTH) */
	*aliased = queue->head->aliased;

	tmp = queue->head;
	queue->head = queue->head->next;
	free(tmp->text);
	free(tmp);

	return (1);
}


/* Empty the queues before closing connection */
void flush_queues(struct descriptor_data *d)
{
	if (d->large_outbuf) {
		release_buffer(d->large_outbuf);
		d->output = d->small_outbuf;	/* necessary? */
	}
	while (d->input.head) {
		struct txt_block *tmp = d->input.head;
		d->input.head = d->input.head->next;
		free(tmp->text);
		free(tmp);
	}
}


/* Add a new string to a player's output queue. For outside use. */
size_t write_to_output(struct descriptor_data *t, int color, const char *txt, ...)
{
	va_list args;
	size_t left;
	
	va_start(args, txt);
	left = vwrite_to_output(t, color, txt, args);
	va_end(args);
	
	return left;
}


/* Add a new string to a player's output queue */
size_t vwrite_to_output(struct descriptor_data *t, int color, const char *format, va_list args)
{
	static char txt[MAX_STRING_LENGTH];
	size_t wantsize;
	int size, level = 0;

	/* if we're in the overflow state already, ignore this new output */
	if (t->bufptr < 0)
		return (0);

	if (t->character)
		level = COLOR_LEV(t->character);

	wantsize = size = vsnprintf(txt, sizeof(txt), format, args);
	if (color)
		wantsize = size = proc_color(txt, level, color, 0, 0);
	if (size < 0 || wantsize >= sizeof(txt))
		size = sizeof(txt) - 1;

	/* if we have enough space, just write to buffer and that's it! */
	if (t->bufspace >= size) {
		strcpy(t->output + t->bufptr, txt);	/* strcpy: OK (size checked above) */
		t->bufspace -= size;
		t->bufptr += size;
		return (t->bufspace);
	}

	/*
	 * If the text is too big to fit into even a large buffer, chuck
	 * the new text, and switch to the overflow state.
	 */
	/* FIXME: This should write a partial string up to what fits! */
	if (size + t->bufptr > LARGE_BUFSIZE - 1) {
		t->bufptr = -1;
		buf_overflows++;
		return (0);
	}
	buf_switches++;

	/*
	 * Just request the buffer. Copy the contents of the old, and make it
	 * the primary buffer.
	 */
	t->large_outbuf = get_buffer(LARGE_BUFSIZE);
	s2b_cpy(t->large_outbuf, t->output);
	t->output = sz(t->large_outbuf);
	strcat(t->output, txt);	/* strcat: OK (size checked) */

	/* calculate how much space is left in the buffer */
	t->bufspace = LARGE_BUFSIZE - 1 - t->bufptr;

	return (t->bufspace);
}



/* ******************************************************************
*	 socket handling                                                  *
****************************************************************** */


/*
 * get_bind_addr: Return a struct in_addr that should be used in our
 * call to bind().  If the user has specified a desired binding
 * address, we try to bind to it; otherwise, we bind to INADDR_ANY.
 * Note that inet_aton() is preferred over inet_addr() so we use it if
 * we can.  If neither is available, we always bind to INADDR_ANY.
 */

struct in_addr *get_bind_addr()
{
	static struct in_addr bind_addr;

	/* Clear the structure */
	memset((char *) &bind_addr, 0, sizeof(bind_addr));

	/* If DLFT_IP is unspecified, use INADDR_ANY */
	if (DFLT_IP == NULL) {
		bind_addr.s_addr = htonl(INADDR_ANY);
	} else {
		/* If the parsing fails, use INADDR_ANY */
		if (!parse_ip(DFLT_IP, &bind_addr)) {
			mlog("SYSERR: DFLT_IP of %s appears to be an invalid IP address",DFLT_IP);
			bind_addr.s_addr = htonl(INADDR_ANY);
		}
	}

	/* Put the address that we've finally decided on into the logs */
	if (bind_addr.s_addr == htonl(INADDR_ANY))
		mlog("Binding to all IP interfaces on this host.");
	else
		mlog("Binding only to IP address %s", inet_ntoa(bind_addr));

	return (&bind_addr);
}

#ifdef HAVE_INET_ATON

/*
 * inet_aton's interface is the same as parse_ip's: 0 on failure, non-0 if
 * successful
 */
int	parse_ip(const char *addr, struct in_addr *inaddr)
{
	return (inet_aton(addr, inaddr));
}

#elif	HAVE_INET_ADDR

/* inet_addr has a different interface, so we emulate inet_aton's */
int	parse_ip(const char *addr, struct in_addr *inaddr)
{
	long ip;

	if ((ip = inet_addr(addr)) == -1) {
		return (0);
	} else {
		inaddr->s_addr = (unsigned long) ip;
		return (1);
	}
}

#else

/* If you have neither function - sorry, you can't do specific binding. */
int	parse_ip(const char *addr, struct in_addr *inaddr)
{
	mlog("SYSERR: warning: you're trying to set DFLT_IP but your system has no "
			"functions to parse IP addresses (how bizarre!)");
	return (0);
}

#endif /* INET_ATON and INET_ADDR */



/* Sets the kernel's send buffer size for the descriptor */
int	set_sendbuf(socket_t s)
{
#if	defined(SO_SNDBUF) && !defined(CIRCLE_MACINTOSH)
	int opt = MAX_SOCK_BUF;

	if (setsockopt(s, SOL_SOCKET, SO_SNDBUF, (char *) &opt, sizeof(opt)) < 0) {
		perror("SYSERR: setsockopt SNDBUF");
		return (-1);
	}

#if	0
	if (setsockopt(s, SOL_SOCKET, SO_RCVBUF, (char *) &opt, sizeof(opt)) < 0) {
		perror("SYSERR: setsockopt RCVBUF");
		return (-1);
	}
#endif

#endif

	return (0);
}


/* Initialize a descriptor */
void init_descriptor (struct descriptor_data *newd, int desc)
{
	newd->descriptor = desc;
	newd->idle_tics = 0;
	newd->output = newd->small_outbuf;
	newd->bufspace = SMALL_BUFSIZE - 1;
	newd->login_time = time(0);
	*newd->output = '\0';
	newd->bufptr = 0;
	newd->has_prompt = 1;  /* prompt is part of greetings */
	STATE(newd) = CON_GET_NAME;
	CREATE(newd->history, char *, HISTORY_SIZE);
	if (++last_desc == 1000)
		last_desc = 1;
	newd->desc_num = last_desc;
}


int	new_descriptor(socket_t s)
{
	socket_t desc;
	int sockets_connected = 0;
	socklen_t i;
	struct descriptor_data *newd;
	struct sockaddr_in peer;
	struct hostent *from;

	/* accept the new connection */
	i = sizeof(peer);
	if ((desc = accept(s, (struct sockaddr *) &peer, &i)) == INVALID_SOCKET) {
		perror("SYSERR: accept");
		return (-1);
	}
	/* keep it from blocking */
	nonblock(desc);

	/* set the send buffer size */
	if (set_sendbuf(desc) < 0) {
		CLOSE_SOCKET(desc);
		return (0);
	}

	/* make sure we have room for it */
	for (newd = descriptor_list; newd; newd = newd->next)
		sockets_connected++;

	if (in_copyover) {
		write_to_descriptor(desc, "Arcane Realms is currently in copyover mode.  Please try again in a few moments.\r\n");
		CLOSE_SOCKET(desc);
		return (0);
	}

	if (sockets_connected >= max_players) {
		write_to_descriptor(desc, "Sorry, Arcane Realms is full right now... please try again later!\r\n");
		CLOSE_SOCKET(desc);
		return (0);
	}
	/* create a new descriptor */
	CREATE(newd, struct descriptor_data, 1);
	memset((char *) newd, 0, sizeof(struct descriptor_data));

	/* find the sitename */
	if (nameserver_is_slow || !(from = gethostbyaddr((char *) &peer.sin_addr,
																			sizeof(peer.sin_addr), AF_INET))) {

		/* resolution failed */
		if (!nameserver_is_slow)
			perror("SYSERR: gethostbyaddr");

		/* find the numeric site address */
		strncpy(newd->host, (char *)inet_ntoa(peer.sin_addr), HOST_LENGTH);	/* strncpy: OK (n->host:HOST_LENGTH+1) */
		*(newd->host + HOST_LENGTH) = '\0';
	} else {
		strncpy(newd->host, from->h_name, HOST_LENGTH);	/* strncpy: OK (n->host:HOST_LENGTH+1) */
		*(newd->host + HOST_LENGTH) = '\0';
	}

	/* determine if the site is banned */
	if (isbanned(newd->host) == BAN_ALL) {
		CLOSE_SOCKET(desc);
		extended_mudlog(BRF, SYSL_LINKS, TRUE, "Connection attempt denied from @%s", newd->host);
		free(newd);
		return (0);
	}
#if	0
	/*
	 * Log new connections - probably unnecessary, but you may want it.
	 * Note that your immortals may wonder if they see a connection from
	 * your site, but you are wizinvis upon login.
	 */
	extended_mudlog(BRF, SYSL_SITES, TRUE, "New connection from @%s", newd->host);
#endif

	/* initialize descriptor data */
	init_descriptor(newd, desc);
	
	/* prepend to list */
	newd->next = descriptor_list;
	descriptor_list = newd;

	write_to_output(newd, FALSE, "%s", GREETINGS);

	return (0);
}


/*
 * Send all of the output that we've accumulated for a player out to
 * the player's descriptor.
 * FIXME - This will be rewritten before 3.1, this code is dumb.
 * FIXME: string nightmare
 *
 * 32 byte GARBAGE_SPACE in MAX_SOCK_BUF used for:
 *    2 bytes: prepended \r\n
 *   14 bytes: overflow message
 *    2 bytes: extra \r\n for non-comapct
 *   14 bytes: unused
 */
int	process_output(struct descriptor_data *t)
{
	char i[MAX_SOCK_BUF];
	int result;

	/* we may need this \r\n for later -- see below */
	strcpy(i, "\r\n");	/* strcpy: OK (for 'MAX_SOCK_BUF >= 3') */

	/* now, append the 'real' output */
	strcpy(i + 2, t->output);	/* strcpy: OK (t->output:LARGE_BUFSIZE < i:MAX_SOCK_BUF-2) */

	/* if we're in the overflow state, notify the user */
	if (t->bufptr < 0)
		strcat(i, "**OVERFLOW**\r\n");	/* strcpy: OK (i:MAX_SOCK_BUF reserves space) */

	/* add the extra CRLF if the person isn't in compact mode */
	if ((STATE(t) == CON_PLAYING || STATE(t) == CON_COPYOVER) && t->character && !IS_NPC(t->character) && !PRF_FLAGGED(t->character, PRF_COMPACT))
		strcat(i, "\r\n");	/* strcpy: OK (i:MAX_SOCK_BUF reserves space) */

	/* add a prompt */
	strcat(i, make_prompt(t));	/* strcpy: OK (i:MAX_SOCK_BUF reserves space) */

	/*
	 * now, send the output.  If this is an 'interruption', use the prepended
	 * CRLF, otherwise send the straight output sans CRLF.
	 */
	if (t->has_prompt)                /* && !t->connected) */
		result = write_to_descriptor(t->descriptor, i);
	else
		result = write_to_descriptor(t->descriptor, i + 2);

	/* handle snooping: prepend "% " and send to snooper */
	if (t->snoop_by)
		write_to_output(t->snoop_by, TRUE, "%% %s%%%%", t->output);

	/*
	 * if we were using a large buffer, put the large buffer on the buffer pool
	 * and switch back to the small one
	 */
	if (t->large_outbuf) {
		release_buffer(t->large_outbuf);
		t->output = t->small_outbuf;
	}
	/* reset total bufspace back to that of a small buffer */
	t->bufspace = SMALL_BUFSIZE - 1;
	t->bufptr = 0;
	*(t->output) = '\0';

	return (result);
}


/*
 * perform_socket_write: takes a descriptor, a pointer to text, and a
 * text length, and tries once to send that text to the OS.  This is
 * where we stuff all the platform-dependent stuff that used to be
 * ugly #ifdef's in write_to_descriptor().
 *
 * This function must return:
 *
 * -1  If a fatal error was encountered in writing to the descriptor.
 *  0  If a transient failure was encountered (e.g. socket buffer full).
 * >0  To indicate the number of bytes successfully written, possibly
 *     fewer than the number the caller requested be written.
 *
 * Right now there are two versions of this function: one for Windows,
 * and one for all other platforms.
 */

#if	defined(CIRCLE_WINDOWS)

ssize_t	perform_socket_write(socket_t desc, const char *txt, size_t length)
{
	ssize_t result;

	result = send(desc, txt, length, 0);

	if (result > 0) {
		/* Write was sucessful */
		return (result);
	}

	if (result == 0) {
		/* This should never happen! */
		mlog("SYSERR: Huh??  write() returned 0???  Please report this!");
		return (-1);
	}

	/* result < 0: An error was encountered. */

	/* Transient error? */
	if (WSAGetLastError() == WSAEWOULDBLOCK || WSAGetLastError() == WSAEINTR)
		return (0);

	/* Must be a fatal error. */
	return (-1);
}

#else

#if	defined(CIRCLE_ACORN)
#define	write        socketwrite
#endif

/* perform_socket_write for all Non-Windows platforms */
ssize_t	perform_socket_write(socket_t desc, const char *txt, size_t length)
{
	ssize_t result;

	result = write(desc, txt, length);

	if (result > 0) {
		/* Write was successful. */
		return (result);
	}

	if (result == 0) {
		/* This should never happen! */
		mlog("SYSERR: Huh??  write() returned 0???  Please report this!");
		return (-1);
	}

	/*
	 * result < 0, so an error was encountered - is it transient?
	 * Unfortunately, different systems use different constants to
	 * indicate this.
	 */

#ifdef EAGAIN                /* POSIX */
	if (errno == EAGAIN)
		return (0);
#endif

#ifdef EWOULDBLOCK        /* BSD */
	if (errno == EWOULDBLOCK)
		return (0);
#endif

#ifdef EDEADLK                /* Macintosh */
	if (errno == EDEADLK)
		return (0);
#endif

	/* Looks like the error was fatal.  Too bad. */
	return (-1);
}

#endif /* CIRCLE_WINDOWS */

		
/*
 * write_to_descriptor takes a descriptor, and text to write to the
 * descriptor.  It keeps calling the system-level write() until all
 * the text has been delivered to the OS, or until an error is
 * encountered.
 *
 * Returns:
 *  0  If all is well and good,
 * -1  If an error was encountered, so that the player should be cut off
 */
int	write_to_descriptor(socket_t desc, const char *txt)
{
	ssize_t bytes_written;
	size_t total = strlen(txt);

	while (total > 0) {
		bytes_written = perform_socket_write(desc, txt, total);

		if (bytes_written < 0) {
			/* Fatal error.  Disconnect the player. */
			perror("SYSERR: Write to socket");
			return (-1);
		} else if (bytes_written == 0) {
			/*
			 * Temporary failure -- socket buffer full.  For now we'll just
			 * cut off the player, but eventually we'll stuff the unsent
			 * text into a buffer and retry the write later.  JE 30 June 98.
			 */
			mlog("WARNING: write_to_descriptor: socket write would block, about to close");
			return (-1);
		} else {
			txt += bytes_written;
			total -= bytes_written;
		}
	}

	return (0);
}


/*
 * Same information about perform_socket_write applies here. I like
 * standards, there are so many of them. -gg 6/30/98
 */
ssize_t	perform_socket_read(socket_t desc, char *read_point, size_t space_left)
{
	ssize_t ret;

#if	defined(CIRCLE_ACORN)
	ret = recv(desc, read_point, space_left, MSG_DONTWAIT);
#elif	defined(CIRCLE_WINDOWS)
	ret = recv(desc, read_point, space_left, 0);
#else
	ret = read(desc, read_point, space_left);
#endif

	/* Read was successful. */
	if (ret > 0)
		return (ret);

	/* read() returned 0, meaning we got an EOF. */
	if (ret == 0) {
		mlog("WARNING: EOF on socket read (connection broken by peer)");
		return (-1);
	}

	/*
	 * read returned a value < 0: there was an error
	 */

#if	defined(CIRCLE_WINDOWS)        /* Windows */
	if (WSAGetLastError() == WSAEWOULDBLOCK || WSAGetLastError() == WSAEINTR)
		return (0);
#else

#ifdef EINTR                /* Interrupted system call - various platforms */
	if (errno == EINTR)
		return (0);
#endif

#ifdef EAGAIN                /* POSIX */
	if (errno == EAGAIN)
		return (0);
#endif

#ifdef EWOULDBLOCK        /* BSD */
	if (errno == EWOULDBLOCK)
		return (0);
#endif /* EWOULDBLOCK */

#ifdef EDEADLK                /* Macintosh */
	if (errno == EDEADLK)
		return (0);
#endif

#ifdef ECONNRESET
	if (errno == ECONNRESET)
		return (-1);
#endif

#endif /* CIRCLE_WINDOWS */

	/*
	 * We don't know what happened, cut them off. This qualifies for
	 * a SYSERR because we have no idea what happened at this point.
	 */
	perror("SYSERR: perform_socket_read: about to lose connection");
	return (-1);
}

/*
 * ASSUMPTION: There will be no newlines in the raw input buffer when this
 * function is called.  We must maintain that before returning.
 *
 * Ever wonder why 'tmp' had '+8' on it?  The crusty old code could write
 * MAX_INPUT_LENGTH+1 bytes to 'tmp' if there was a '$' as the final
 * character in the input buffer.  This would also cause 'space_left' to
 * drop to -1, which wasn't very happy in an unsigned variable.  Argh.
 * So to fix the above, 'tmp' lost the '+8' since it doesn't need it
 * and the code has been changed to reserve space by accepting one less
 * character. (Do you really need 256 characters on a line?)
 * -gg 1/21/2000
 */
int	process_input(struct descriptor_data *t)
{
	int buf_length, failed_subst;
	ssize_t bytes_read;
	size_t space_left;
	char *ptr, *read_point, *write_point, *nl_pos = NULL;
	char tmp[MAX_INPUT_LENGTH];

	/* first, find the point where we left off reading data */
	buf_length = strlen(t->inbuf);
	read_point = t->inbuf + buf_length;
	space_left = MAX_RAW_INPUT_LENGTH - buf_length - 1;

	do {
		if (space_left <= 0) {
			mlog("WARNING: process_input: about to close connection: input overflow");
			return (-1);
		}

		bytes_read = perform_socket_read(t->descriptor, read_point, space_left);

		if (bytes_read < 0)        /* Error, disconnect them. */
			return (-1);
		else if (bytes_read == 0)        /* Just blocking, no problems. */
			return (0);

		/* at this point, we know we got some data from the read */

		*(read_point + bytes_read) = '\0';        /* terminate the string */

		/* search for a newline in the data we just read */
		for (ptr = read_point; *ptr && !nl_pos; ptr++)
			if (ISNEWL(*ptr))
				nl_pos = ptr;

		read_point += bytes_read;
		space_left -= bytes_read;

/*
 * on some systems such as AIX, POSIX-standard nonblocking I/O is broken,
 * causing the MUD to hang when it encounters input not terminated by a
 * newline.  This was causing hangs at the Password: prompt, for example.
 * I attempt to compensate by always returning after the _first_ read, instead
 * of looping forever until a read returns -1.  This simulates non-blocking
 * I/O because the result is we never call read unless we know from select()
 * that data is ready (process_input is only called if select indicates that
 * this descriptor is in the read set).  JE 2/23/95.
 */
#if	!defined(POSIX_NONBLOCK_BROKEN)
	} while (nl_pos == NULL);
#else
	} while (0);

	if (nl_pos == NULL)
		return (0);
#endif /* POSIX_NONBLOCK_BROKEN */

	/*
	 * okay, at this point we have at least one newline in the string; now we
	 * can copy the formatted data to a new array for further processing.
	 */

	read_point = t->inbuf;

	while (nl_pos != NULL) {
		write_point = tmp;
		space_left = MAX_INPUT_LENGTH - 1;

		/* The '> 1' reserves room for a '$ => $$' expansion. */
		for (ptr = read_point; (space_left > 1) && (ptr < nl_pos); ptr++) {
			if (*ptr == '\b' || *ptr == 127) { /* handle backspacing or delete key */
				if (write_point > tmp) {
					if (*(--write_point) == '$') {
						write_point--;
						space_left += 2;
					} else
						space_left++;
				}
			} else if (isascii(*ptr) && isprint(*ptr)) {
				if ((*(write_point++) = *ptr) == '$') {                /* copy one character */
					*(write_point++) = '$';        /* if it's a $, double it */
					space_left -= 2;
				} else
					space_left--;
			}
		}

		*write_point = '\0';

		if ((space_left <= 0) && (ptr < nl_pos)) {
			char buffer[MAX_INPUT_LENGTH + 64];

			snprintf(buffer, sizeof(buffer), "Line too long.  Truncated to:\r\n%s\r\n", tmp);
			if (write_to_descriptor(t->descriptor, buffer) < 0)
				return (-1);
		}
		if (t->snoop_by)
			write_to_output(t->snoop_by, TRUE, "%% %s\r\n", tmp);
		failed_subst = 0;

		if (*tmp == '!' && !(*(tmp + 1)))        /* Redo last command. */
			strcpy(tmp, t->last_input);	/* strcpy: OK (by mutual MAX_INPUT_LENGTH) */
		else if (*tmp == '!' && *(tmp + 1)) {
			char *commandln = (tmp + 1);
			int starting_pos = t->history_pos,
					cnt = (t->history_pos == 0 ? HISTORY_SIZE - 1 : t->history_pos - 1);

			skip_spaces(&commandln);
			for (; cnt != starting_pos; cnt--) {
				if (t->history[cnt] && is_abbrev(commandln, t->history[cnt])) {
					strcpy(tmp, t->history[cnt]);	/* strcpy: OK (by mutual MAX_INPUT_LENGTH) */
					strcpy(t->last_input, tmp);	/* strcpy: OK (by mutual MAX_INPUT_LENGTH) */
					write_to_output(t, TRUE, "%s\r\n", tmp);
					break;
				}
				if (cnt == 0)        /* At top, loop to bottom. */
					cnt = HISTORY_SIZE;
			}
		} else if (*tmp == '^') {
			if (!(failed_subst = perform_subst(t, t->last_input, tmp)))
				strcpy(t->last_input, tmp);	/* strcpy: OK (by mutual MAX_INPUT_LENGTH) */
		} else {
			strcpy(t->last_input, tmp);	/* strcpy: OK (by mutual MAX_INPUT_LENGTH) */
			if (t->history[t->history_pos])
				free(t->history[t->history_pos]);					/* Clear the old line. */
			t->history[t->history_pos] = str_dup(tmp);	/* Save the new. */
			if (++t->history_pos >= HISTORY_SIZE)				/* Wrap to top. */
				t->history_pos = 0;
		}

		if (!failed_subst)
			write_to_q(tmp, &t->input, 0);

		/* find the end of this line */
		while (ISNEWL(*nl_pos))
			nl_pos++;

		/* see if there's another newline in the input buffer */
		read_point = ptr = nl_pos;
		for (nl_pos = NULL; *ptr && !nl_pos; ptr++)
			if (ISNEWL(*ptr))
				nl_pos = ptr;
	}

	/* now move the rest of the buffer up to the beginning for the next pass */
	write_point = t->inbuf;
	while (*read_point)
		*(write_point++) = *(read_point++);
	*write_point = '\0';

	return (1);
}


/* perform substitution for the '^..^' csh-esque syntax orig is the
 * orig string, i.e. the one being modified.  subst contains the
 * substition string, i.e. "^telm^tell"
 */
int	perform_subst(struct descriptor_data *t, char *orig, char *subst)
{
	char newsub[MAX_INPUT_LENGTH + 5];

	char *first, *second, *strpos;

	/*
	 * first is the position of the beginning of the first string (the one
	 * to be replaced
	 */
	first = subst + 1;

	/* now find the second '^' */
	if (!(second = strchr(first, '^'))) {
		write_to_output(t, FALSE, "Invalid substitution.\r\n");
		return (1);
	}
	/* terminate "first" at the position of the '^' and make 'second' point
	 * to the beginning of the second string */
	*(second++) = '\0';

	/* now, see if the contents of the first string appear in the original */
	if (!(strpos = strstr(orig, first))) {
		write_to_output(t, FALSE, "Invalid substitution.\r\n");
		return (1);
	}
	/* now, we construct the new string for output. */

	/* first, everything in the original, up to the string to be replaced */
	strncpy(newsub, orig, strpos - orig);	/* strncpy: OK (newsub:MAX_INPUT_LENGTH+5 > orig:MAX_INPUT_LENGTH) */
	newsub[strpos - orig] = '\0';

	/* now, the replacement string */
	strncat(newsub, second, MAX_INPUT_LENGTH - strlen(newsub) - 1);	/* strncpy: OK */

	/* now, if there's anything left in the original after the string to
	 * replaced, copy that too. */
	if (((strpos - orig) + strlen(first)) < strlen(orig))
		strncat(newsub, strpos + strlen(first), MAX_INPUT_LENGTH - strlen(newsub) - 1);	/* strncpy: OK */

	/* terminate the string in case of an overflow from strncat */
	newsub[MAX_INPUT_LENGTH - 1] = '\0';
	strcpy(subst, newsub);	/* strcpy: OK (by mutual MAX_INPUT_LENGTH) */

	return (0);
}


void close_socket(struct descriptor_data *d)
{
	struct descriptor_data *temp;

	REMOVE_FROM_LIST(d, descriptor_list, next);
	CLOSE_SOCKET(d->descriptor);
	flush_queues(d);

	/* Forget snooping */
	if (d->snooping)
		d->snooping->snoop_by = NULL;

	if (d->snoop_by) {
		write_to_output(d->snoop_by, FALSE, "Your victim is no longer among us.\r\n");
		d->snoop_by->snooping = NULL;
	}

	/* Free edit in progress if there is one */
	if (EDITING(d))
		 free_editor(d);

	if (d->character) {
		/* If we're switched, this resets the mobile taken. */
		d->character->desc = NULL;
		
		if (STATE(d) == CON_PLAYING || STATE(d) == CON_COPYOVER || STATE(d) == CON_DISCONNECT || IN_OLC(d)) {
			struct char_data *link_challenged = d->original ? d->original : d->character;

			/* We are guaranteed to have a person. */
			act("$n has lost $s link.", TRUE, link_challenged, 0, 0, TO_ROOM);
			save_char(link_challenged, NOWHERE, FALSE);
			extended_mudlog(BRF, SYSL_LINKS, TRUE, "Closing link to %s@%s.", GET_NAME(link_challenged), GET_HOST(link_challenged));
		} else {
			extended_mudlog(BRF, SYSL_LINKS, TRUE, "Losing player %s@%s.",
							GET_NAME(d->character) ? GET_NAME(d->character) : "nobody",
							d->host ? d->host : "nowhere");
			free_char(d->character);
		}
	} else
		extended_mudlog(BRF, SYSL_LINKS, TRUE, "Losing descriptor without char @%s.", d->host ? d->host : "nowhere");

	/* JE 2/22/95 -- part of my unending quest to make switch stable */
	if (d->original && d->original->desc)
		d->original->desc = NULL;

	/*
	 * As long as we are not in emergency reboot state, we clear up memory after us.
	 * In the case that we are doing an emergency shutdown/reboot (circle_reboot = 3),
	 * we shouldn't call the buffer system at all, or nasty things(tm) might happen.
	 * Torgny Bjers (artovil@arcanerealms.org) 2002-08-27, 11:03:18
	 */
	if (circle_reboot < 3) {

		/* Clear the command history. */
		if (d->history) {
			int cnt;
			for (cnt = 0; cnt < HISTORY_SIZE; cnt++)
				if (d->history[cnt])
					free(d->history[cnt]);
				free(d->history);
		}

		if (d->showstr_head)
			free(d->showstr_head);
		if (d->showstr_count)
			free(d->showstr_vector);

		/*. Kill any OLC stuff .*/
		switch (d->connected) {
			case CON_OEDIT:
			case CON_REDIT:
			case CON_ZEDIT:
			case CON_MEDIT:
			case CON_SEDIT:
			case CON_AEDIT:
			case CON_TRIGEDIT:
			case CON_QEDIT:
			case CON_GEDIT:
			case CON_SPEDIT:
			case CON_COMEDIT:
			case CON_TUTOREDIT:
				cleanup_olc(d, CLEANUP_ALL);
				break;
			case CON_EMAIL:
				if (OLC(d))
					free(d->olc);
					d->olc = NULL;
				break;
			default:
				break;
		}

		free(d);

	}
}


void check_idle_passwords(void)
{
	struct descriptor_data *d, *next_d;

	for (d = descriptor_list; d; d = next_d) {
		next_d = d->next;
		if (STATE(d) != CON_PASSWORD && STATE(d) != CON_GET_NAME)
			continue;
		if (!d->idle_tics) {
			d->idle_tics++;
			continue;
		} else {
			echo_on(d);
			write_to_output(d, FALSE, "\r\nTimed out... goodbye.\r\n");
			STATE(d) = CON_CLOSE;
		}
	}
}


/*
 * I tried to universally convert Circle over to POSIX compliance, but
 * alas, some systems are still straggling behind and don't have all the
 * appropriate defines.  In particular, NeXT 2.x defines O_NDELAY but not
 * O_NONBLOCK.  Krusty old NeXT machines!  (Thanks to Michael Jones for
 * this and various other NeXT fixes.)
 */

#if	defined(CIRCLE_WINDOWS)

void nonblock(socket_t s)
{
	unsigned long val = 1;
	ioctlsocket(s, FIONBIO, &val);
}

#elif	defined(CIRCLE_AMIGA)

void nonblock(socket_t s)
{
	long val = 1;
	IoctlSocket(s, FIONBIO, &val);
}

#elif	defined(CIRCLE_ACORN)

void nonblock(socket_t s)
{
	int val = 1;
	socket_ioctl(s, FIONBIO, &val);
}

#elif	defined(CIRCLE_VMS)

void nonblock(socket_t s)
{
	int val = 1;

	if (ioctl(s, FIONBIO, &val) < 0) {
		perror("SYSERR: Fatal error executing nonblock (comm.c)");
		exit(1);
	}
}

#elif	defined(CIRCLE_UNIX) || defined(CIRCLE_OS2) || defined(CIRCLE_MACINTOSH)

#ifndef	O_NONBLOCK
#define	O_NONBLOCK O_NDELAY
#endif

void nonblock(socket_t s)
{
	int flags;

	flags = fcntl(s, F_GETFL, 0);
	flags |= O_NONBLOCK;
	if (fcntl(s, F_SETFL, flags) < 0) {
		perror("SYSERR: Fatal error executing nonblock (comm.c)");
		exit(1);
	}
}

#endif	/* CIRCLE_UNIX || CIRCLE_OS2 || CIRCLE_MACINTOSH */


/* ******************************************************************
*	 signal-handling functions (formerly signals.c).  UNIX only.      *
****************************************************************** */

#if	defined(CIRCLE_UNIX) || defined(CIRCLE_MACINTOSH)

RETSIGTYPE reread_wizlists(int sig)
{
	reread_wizlist = TRUE;
}


RETSIGTYPE unrestrict_game(int sig)
{
	emergency_unban = TRUE;
}

#ifdef CIRCLE_UNIX

/* clean up our zombie kids to avoid defunct processes */
RETSIGTYPE reap(int sig)
{
	while (waitpid(-1, NULL, WNOHANG) > 0);

	my_signal(SIGCHLD, reap);
}


/* Dying anyway... */
RETSIGTYPE checkpointing(int sig)
{
	if (!tics) {
		mlog("SYSERR: CHECKPOINT shutdown: tics not updated. (Infinite loop suspected)");
		abort();
	} else
		tics = 0;
}


/* Dying anyway... */
RETSIGTYPE hupsig(int sig)
{
	mlog("SYSERR: Received SIGHUP, SIGINT, or SIGTERM.  Shutting down...");
	exit(1);                        /* perhaps something more elegant should
																 * substituted */
}

#endif				/* CIRCLE_UNIX */

/*
 * This is an implementation of signal() using sigaction() for portability.
 * (sigaction() is POSIX; signal() is not.)  Taken from Stevens' _Advanced
 * Programming in the UNIX Environment_.  We are specifying that all system
 * calls _not_ be automatically restarted for uniformity, because BSD systems
 * do not restart select(), even if SA_RESTART is used.
 *
 * Note that NeXT 2.x is not POSIX and does not have sigaction; therefore,
 * I just define it to be the old signal.  If your system doesn't have
 * sigaction either, you can use the same fix.
 *
 * SunOS Release 4.0.2 (sun386) needs this too, according to Tim Aldric.
 */

#ifndef	POSIX
#define	my_signal(signo, func) signal(signo, func)
#else
sigfunc	*my_signal(int signo, sigfunc *func)
{
	struct sigaction sact, oact;

	sact.sa_handler = func;
	sigemptyset(&sact.sa_mask);
	sact.sa_flags = 0;
#ifdef SA_INTERRUPT
	sact.sa_flags |= SA_INTERRUPT;	/* SunOS */
#endif

	if (sigaction(signo, &sact, &oact) < 0)
		return (SIG_ERR);

	return (oact.sa_handler);
}
#endif																/* POSIX */


void signal_setup(void)
{
#ifndef	CIRCLE_MACINTOSH
	struct itimerval itime;
	struct timeval interval;

	/* user signal 1: reread wizlists.  Used by autowiz system. */
	my_signal(SIGUSR1, reread_wizlists);

	/*
	 * user signal 2: unrestrict game.  Used for emergencies if you lock
	 * yourself out of the MUD somehow.  (Duh...)
	 */
	my_signal(SIGUSR2, unrestrict_game);

	/*
	 * set up the deadlock-protection so that the MUD aborts itself if it gets
	 * caught in an infinite loop for more than 3 minutes.
	 */
	interval.tv_sec = 180;
	interval.tv_usec = 0;
	itime.it_interval = interval;
	itime.it_value = interval;
	setitimer(ITIMER_VIRTUAL, &itime, NULL);
	my_signal(SIGVTALRM, checkpointing);

	/* just to be on the safe side: */
	my_signal(SIGHUP, hupsig);
	my_signal(SIGCHLD, reap);
#endif /* CIRCLE_MACINTOSH */
	my_signal(SIGINT, hupsig);
	my_signal(SIGTERM, hupsig);
	my_signal(SIGPIPE, SIG_IGN);
	my_signal(SIGALRM, SIG_IGN);
}

#endif				/* CIRCLE_UNIX || CIRCLE_MACINTOSH */

/* ****************************************************************
*				Public routines for system-to-player-communication        *
**************************************************************** */

void send_to_char(const char *messg, struct char_data *ch)
{
	if (ch->desc && messg)
		write_to_output(ch->desc, TRUE, "%s", messg);
}


size_t send_to_charf(struct char_data *ch, const char *messg, ...)
{
	if (ch->desc && messg && *messg) {
		size_t left;
		va_list args;
	
		va_start(args, messg);
		left = vwrite_to_output(ch->desc, TRUE, messg, args);
		va_end(args);
		return left;
	}
	return 0;
}


void send_to_all(const char *messg)
{
	struct descriptor_data *i;

	if (messg == NULL)
		return;

	for (i = descriptor_list; i; i = i->next)
		if (STATE(i) == CON_PLAYING || STATE(i) == CON_COPYOVER)
			write_to_output(i, TRUE, "%s", messg);
}


void send_to_allf(const char *messg, ...)
{
	struct descriptor_data *i;
	va_list args;
	
	 if (messg == NULL)
		 return;
	
	for (i = descriptor_list; i; i = i->next) {
		if (STATE(i) != CON_PLAYING || STATE(i) != CON_COPYOVER)
			continue;
	
		va_start(args, messg);
		vwrite_to_output(i, TRUE, messg, args);
		va_end(args);
	}
}


void send_to_outdoor(const char *messg)
{
	struct descriptor_data *i;

	if (!messg || !*messg)
		return;

	for (i = descriptor_list; i; i = i->next) {
		if (!(STATE(i) == CON_PLAYING || STATE(i) == CON_COPYOVER) || i->character == NULL)
			continue;
		if (!AWAKE(i->character) || !OUTSIDE(i->character) || EDITING(i))
			continue;
		write_to_output(i, TRUE, "%s", messg);
	}
}


void send_to_outdoorf(const char *messg, ...)
{
	struct descriptor_data *i;

	if (!messg || !*messg)
		return;

	for (i = descriptor_list; i; i = i->next) {
		va_list args;
		
		if (!(STATE(i) == CON_PLAYING || STATE(i) == CON_COPYOVER) || i->character == NULL)
			continue;
		if (!AWAKE(i->character) || !OUTSIDE(i->character) || EDITING(i))
			continue;
		
		va_start(args, messg);
		vwrite_to_output(i, TRUE, messg, args);
		va_end(args);
	}
}


void send_to_room(const char *messg, room_rnum room)
{
	struct char_data *i;

	if (messg == NULL)
		return;

	for (i = world[room].people; i; i = i->next_in_room)
		if (i->desc)
			write_to_output(i->desc, TRUE, "%s", messg);
}


void send_to_roomf(room_rnum room, const char *messg, ...)
{
	struct char_data *i;
	va_list args;

	if (messg == NULL)
		return;

	for (i = world[room].people; i; i = i->next_in_room) {
		va_start(args, messg);
		vwrite_to_output(i->desc, TRUE, messg, args);
		va_end(args);
	}
}


const	char *ACTNULL = "<NULL>";

#define	CHECK_NULL(pointer, expression) \
	if ((pointer) == NULL) i = ACTNULL; else i = (expression);


/* higher-level communication: the act() function */
void perform_act(const char *orig, struct char_data *ch, struct obj_data *obj,
								const void *vict_obj, const struct char_data *to, int use_name)
{
	const char *i = NULL;
	char lbuf[MAX_STRING_LENGTH], *buf, *j;
	bool uppercasenext = FALSE;
	struct char_data *dg_victim = NULL;
	struct obj_data *dg_target = NULL;
	char *dg_arg = NULL;
	int ooc_name = 0;

	buf = lbuf;
  
	for (;;) {
		ooc_name = use_name;
		if (*orig == '$') {
			switch (*(++orig)) {
			case 'n':
				if (find_recognized((struct char_data *)to, ch))
					ooc_name += 1;
				i = PERS(ch, to, ooc_name);
				break;
			case 'N':
				if (find_recognized((struct char_data *)to, (struct char_data *)vict_obj))
					ooc_name += 1;
				CHECK_NULL(vict_obj, PERS((const struct char_data *) vict_obj, to, ooc_name));
				dg_victim = (struct char_data *) vict_obj;
				break;
			case 'm':
				i = HMHR(ch);
				break;
			case 'M':
				CHECK_NULL(vict_obj, HMHR((const struct char_data *) vict_obj));
				dg_victim = (struct char_data *) vict_obj;
				break;
			case 's':
				i = HSHR(ch);
				break;
			case 'S':
				CHECK_NULL(vict_obj, HSHR((const struct char_data *) vict_obj));
				dg_victim = (struct char_data *) vict_obj;
				break;
			case 'e':
				i = HSSH(ch);
				break;
			case 'E':
				CHECK_NULL(vict_obj, HSSH((const struct char_data *) vict_obj));
				dg_victim = (struct char_data *) vict_obj;
				break;
			case 'o':
				CHECK_NULL(obj, OBJN(obj, to));
				break;
			case 'O':
				CHECK_NULL(vict_obj, OBJN((const struct obj_data *) vict_obj, to));
				dg_target = (struct obj_data *) vict_obj;
				break;
			case 'p':
				CHECK_NULL(obj, OBJS(obj, to));
				break;
			case 'P':
				CHECK_NULL(vict_obj, OBJS((const struct obj_data *) vict_obj, to));
				dg_target = (struct obj_data *) vict_obj;
				break;
			case 'a':
				CHECK_NULL(obj, SANA(obj));
				break;
			case 'A':
				CHECK_NULL(vict_obj, SANA((const struct obj_data *) vict_obj));
				dg_victim = (struct char_data *) vict_obj;
				break;
			case 'T':
				CHECK_NULL(vict_obj, (const char *) vict_obj);
				dg_arg = (char *) vict_obj;
				break;
			case 't':
				CHECK_NULL(obj, (char *) obj);
				break;
			case 'F':
				CHECK_NULL(vict_obj, fname((const char *) vict_obj));
				break;
			/* uppercase previous word */
			case 'u':
				for (j=buf; j > lbuf && !isspace((int) *(j-1)); j--);
				if (j != buf)
					*j = UPPER(*j);
				i = "";
				break;
			/* uppercase next word */
			case 'U':
				uppercasenext = TRUE;
				i = "";
				break;
			case '$':
				i = "$";
				break;
			default:
				mlog("SYSERR: Illegal $-code to act(): %c", *orig);
				mlog("SYSERR: %s", orig);
				i = "";
				break;
			}
			while ((*buf = *(i++))) {
				if (uppercasenext && !isspace((int) *buf)) {
					*buf = UPPER(*buf);
					uppercasenext = FALSE;
				}
				buf++;
			}
			orig++;
		} else if (!(*(buf++) = *(orig++))) {
			break;
		} else if (uppercasenext && !isspace((int) *(buf-1))) {
			*(buf-1) = UPPER(*(buf-1));
			uppercasenext = FALSE;
		}
	}

	*(--buf) = '\r';
	*(++buf) = '\n';
	*(++buf) = '\0';

	if (to->desc)
		write_to_output(to->desc, TRUE, "%s", CAP(lbuf));

	if ((IS_NPC(to) && dg_act_check) && (to != ch))
		act_mtrigger(to, lbuf, ch, dg_victim, obj, dg_target, dg_arg);
}


void act(const char *str, int hide_invisible, struct char_data *ch,
				 struct obj_data *obj, const void *vict_obj, int type)
{
	const struct char_data *to;
	int to_sleeping, use_name;

	if (!str || !*str)
		return;

	if (!(dg_act_check = !(type & DG_NO_TRIG)))
		type &= ~DG_NO_TRIG;

	/*
	 * Warning: the following TO_SLEEP code is a hack.
	 * 
	 * I wanted to be able to tell act to deliver a message regardless of sleep
	 * without adding an additional argument.  TO_SLEEP is 128 (a single bit
	 * high up).  It's ONLY legal to combine TO_SLEEP with one other TO_x
	 * command.  It's not legal to combine TO_x's with each other otherwise.
	 * TO_SLEEP only works because its value "happens to be" a single bit;
	 * do not change it to something else.  In short, it is a hack.
	 */

	if ((use_name = (type & TO_OOC)))
		type &= ~TO_OOC;

	/* check if TO_SLEEP is there, and remove it if it is. */
	if ((to_sleeping = (type & TO_SLEEP)))
		type &= ~TO_SLEEP;

	if (type == TO_CHAR) {
		if (ch && SENDOK(ch))
			perform_act(str, ch, obj, vict_obj, ch, use_name);
		return;
	}

	if (type == TO_VICT) {
		if ((to = (const struct char_data *) vict_obj) != NULL && SENDOK(to))
			perform_act(str, ch, obj, vict_obj, to, use_name);
		return;
	}

	if (type == TO_GROUP) {
		struct char_data *k;
		struct follow_type *f;
		if (!ch) {
			mlog("SYSERR: act() called with TO_GROUP but no char was found!");
			return;
		}
		if (ch->master)
			k = ch->master;
		else
			k = ch;
		if (AFF_FLAGGED(k, AFF_GROUP) && (k != ch))
			perform_act(str, ch, obj, vict_obj, k, use_name);
		for (f = k->followers; f; f = f->next)
			if (AFF_FLAGGED(f->follower, AFF_GROUP) && (f->follower != ch))
				perform_act(str, ch, obj, vict_obj, f->follower, use_name);
		return;
	}

	/* ASSUMPTION: at this point we know type must be TO_NOTVICT or TO_ROOM */

	if (ch && IN_ROOM(ch) != NOWHERE)
		to = world[IN_ROOM(ch)].people;
	else if (obj && IN_ROOM(obj) != NOWHERE)
		to = world[IN_ROOM(obj)].people;
	else {
		mlog("SYSERR: no valid target to act()!");
		return;
	}

	for (; to; to = to->next_in_room) {
		if (!SENDOK(to) || (to == ch))
			continue;
		if (hide_invisible && ch && !CAN_SEE(to, ch))
			continue;
		if (type != TO_ROOM && to == vict_obj)
			continue;
		perform_act(str, ch, obj, vict_obj, to, use_name);
	}
}

/*
 * This function is called every 30 seconds from heartbeat().  It checks
 * the four global buffers in CircleMUD to ensure that no one has written
 * past their bounds.  If our check digit is not there (and the position
 * doesn't have a NUL which may result from snprintf) then we gripe that
 * someone has overwritten our buffer.  This could cause a false positive
 * if someone uses the buffer as a non-terminated character array but that
 * is not likely. -gg
 */
void sanity_check(void)
{
	int ok = TRUE;

	/*
	 * If any line is false, 'ok' will become false also.
	 */
	ok &= (test_magic(buf)  == MAGIC_NUMBER || test_magic(buf)  == '\0');
	ok &= (test_magic(buf1) == MAGIC_NUMBER || test_magic(buf1) == '\0');
	ok &= (test_magic(buf2) == MAGIC_NUMBER || test_magic(buf2) == '\0');
	ok &= (test_magic(arg)  == MAGIC_NUMBER || test_magic(arg)  == '\0');

	/*
	 * This isn't exactly the safest thing to do (referencing known bad memory)
	 * but we're doomed to crash eventually, might as well try to get something
	 * useful before we go down. -gg
	 * However, lets fix the problem so we don't spam the logs. -gg 11/24/98
	 */
	if (!ok) {
		mlog("SYSERR: *** Buffer overflow! ***\n"
				"buf: %s\nbuf1: %s\nbuf2: %s\narg: %s", buf, buf1, buf2, arg);

		plant_magic(buf);
		plant_magic(buf1);
		plant_magic(buf2);
		plant_magic(arg);
	}

#if	0
	mlog("Statistics: buf=%d buf1=%d buf2=%d arg=%d",
				strlen(buf), strlen(buf1), strlen(buf2), strlen(arg));
#endif
}

/* Prefer the file over the descriptor. */
void setup_log(const char *filename, int fd)
{
	FILE *s_fp;

#if	defined(__MWERKS__) || defined(__GNUC__)
	s_fp = stderr;
#else
	if ((s_fp = fdopen(STDERR_FILENO, "w")) == NULL) {
		puts("SYSERR: Error opening stderr, trying stdout.");

		if ((s_fp = fdopen(STDOUT_FILENO, "w")) == NULL) {
			puts("SYSERR: Error opening stdout, trying a file.");

			/* If we don't have a file, try a default. */
			if (filename == NULL || *filename == '\0')
				filename = "log/syslog";
		}
	}
#endif

	if (filename == NULL || *filename == '\0') {
		/* No filename, set us up with the descriptor we just opened. */
		logfile = s_fp;
		puts("Using file descriptor for logging.");
		return;
	}

	/* We honor the default filename first. */
	if (open_logfile(filename, s_fp))
		return;

	/* Well, that failed but we want it logged to a file so try a default. */
	if (open_logfile("log/syslog", s_fp))
		return;

	/* Ok, one last shot at a file. */
	if (open_logfile("syslog", s_fp))
		return;

	/* Erp, that didn't work either, just die. */
	puts("SYSERR: Couldn't open anything to log to, giving up.");
	exit(1);
}

int	open_logfile(const char *filename, FILE *stderr_fp)
{
	if (stderr_fp)        /* freopen() the descriptor. */
		logfile = freopen(filename, "w", stderr_fp);
	else
		logfile = fopen(filename, "w");

	if (logfile) {
		printf("Using log file '%s'%s.\n",
								filename, stderr_fp ? " with redirection" : "");
		return (TRUE);
	}

	printf("SYSERR: Error opening file '%s': %s\n", filename, strerror(errno));
	return (FALSE);
}

/*
 * This may not be pretty but it keeps game_loop() neater than if it was inline.
 */
#if	defined(CIRCLE_WINDOWS)
 
void circle_sleep(struct timeval *timeout)
{
	Sleep(timeout->tv_sec * 1000 + timeout->tv_usec / 1000);
}
 
#else

void circle_sleep(struct timeval *timeout)
{
	if (select(0, (fd_set *) 0, (fd_set *) 0, (fd_set *) 0, timeout) < 0) {
		if (errno != EINTR) {
			perror("SYSERR: Select sleep");
			exit(1);
		}
	}
}

#endif /* CIRCLE_WINDOWS */
