/*
 *   chfn.c -- change your finger information
 *   (c) 1994 by Salvatore Valente <svalente@mit.edu>
 *
 *   This program is free software. You can redistribute it and
 *   modify it under the terms of the GNU General Public License.
 *   There is no warranty.
 */

#include <sys/types.h>
#include <sys/stat.h>
#include <stdio.h>
#include <string.h>
#include <stdlib.h>
#include <unistd.h>
#include <pwd.h>
#include <errno.h>
#include <ctype.h>
#include <getopt.h>

typedef unsigned char boolean;
#define false 0
#define true 1

static char *version_string = "chfn 0.92";
static char *whoami;

struct finfo {
    struct passwd *pw;
    char *username;
    char *full_name;
    char *office;
    char *office_phone;
    char *home_phone;
    char *other;
};

static boolean parse_argv(int argc, char *argv[], struct finfo *pinfo);
static void usage(FILE *fp);
static void parse_passwd(struct passwd *pw, struct finfo *pinfo);
static void ask_info(struct finfo *oldfp, struct finfo *newfp);
static char *prompt(char *question, char *def_val);
static int check_gecos_string(char *msg, char *gecos);
static boolean set_changed_data(struct finfo *oldfp, struct finfo *newfp);
static int save_new_data(struct finfo *pinfo);
extern int setpwnam(struct passwd *pwd);

int main(int argc, char *argv[])
{
    char *cp;
    uid_t uid;
    struct finfo oldf, newf;
    boolean interactive;
    int status;
    extern int errno;

    /* whoami is the program name for error messages */
    whoami = argv[0];
    if (! whoami) whoami = "chfn";
    for (cp = whoami; *cp; cp++)
        if (*cp == '/') whoami = cp + 1;

    umask (022);

    /*
     *  "oldf" contains the users original finger information.
     *  "newf" contains the changed finger information, and contains NULL
     *         in fields that haven't been changed.
     *  in the end, "newf" is folded into "oldf".
     *  the reason the new finger information is not put _immediately_ into
     *  "oldf" is that on the command line, new finger information can
     *  be specified before we know what user the information is being
     *  specified for.
     */
    uid = getuid ();
    memset (&oldf, 0, sizeof (oldf));
    memset (&newf, 0, sizeof (newf));

    interactive = parse_argv (argc, argv, &newf);
    if (! newf.username) {
        parse_passwd (getpwuid (uid), &oldf);
        if (! oldf.username) {
            fprintf (stderr, "%s: you (user %d) don't exist.\n", whoami, uid);
            return (-1);
        }
    } else {
        parse_passwd (getpwnam (newf.username), &oldf);
        if (! oldf.username) {
            cp = newf.username;
            fprintf (stderr, "%s: user \"%s\" does not exist.\n", whoami, cp);
            return (-1);
        }
    }

    /* reality check */
    if (uid != 0 && uid != oldf.pw->pw_uid) {
        errno = EACCES;
        perror (whoami);
        return (-1);
    }
    
    if (interactive) ask_info (&oldf, &newf);

    if (! set_changed_data (&oldf, &newf)) {
        printf ("Finger information not changed.\n");
        return 0;
    }
    status = save_new_data (&oldf);
    return status;
}

/*
 *  parse_argv () --
 *      parse the command line arguments.
 *      returns true if no information beyond the username was given.
 */
static boolean parse_argv(int argc, char *argv[], struct finfo *pinfo)
{
    int index, c, status;
    boolean info_given;
    char *msg;

    static struct option long_options[] = {
        { "full-name",    required_argument, 0, 'f' },
        { "office",       required_argument, 0, 'o' },
        { "office-phone", required_argument, 0, 'p' },
        { "home-phone",   required_argument, 0, 'h' },
        { "help",         no_argument,       0, 'u' },
        { "version",      no_argument,       0, 'v' },
        { NULL,           no_argument,       0, '0' },
    };

    optind = 0;
    info_given = false;
    while (true) {
        c = getopt_long (argc, argv, "f:r:p:h:o:uv", long_options, &index);
        if (c == EOF) break;
        /* version?  output version and exit. */
        if (c == 'v') {
            printf ("%s\n", version_string);
            exit (0);
        }
        if (c == 'u') {
            usage (stdout);
            exit (0);
        }
        /* all other options must have an argument. */
        if (! optarg) {
            usage (stderr);
            exit (-1);
        }
        /* ok, we were given an argument */
        info_given = true;
        status = 0;

        /* now store the argument */
        switch (c) {
        case 'f':
            pinfo->full_name = optarg;
            msg ="full name";
            status = check_gecos_string (msg, optarg);
            break;
        case 'o':
            pinfo->office = optarg;
            msg ="office";
            status = check_gecos_string (msg, optarg);
            break;
        case 'p':
            pinfo->office_phone = optarg;
            msg ="office phone";
            status = check_gecos_string (msg, optarg);
            break;
        case 'h':
            pinfo->home_phone = optarg;
            msg ="home phone";
            status = check_gecos_string (msg, optarg);
            break;
        default:
            usage (stderr);
            status = (-1);
        }
        if (status < 0) exit (status);
    }
    /* done parsing arguments. check for a username. */
    if (optind < argc) {
        if (optind + 1 < argc) {
            usage (stderr);
            exit (-1);
        }
        pinfo->username = argv[optind];
    }
    return (! info_given);
}

/*
 *  usage () --
 *      print out a usage message.
 */
static void usage(FILE *fp)
{
    fprintf (fp, "Usage: %s [ -f full-name ] [ -o office ] ", whoami);
    fprintf (fp, "[ -p office-phone ]\n [ -h home-phone ] ");
    fprintf (fp, "[ --help ] [ --version ]\n");
}

/*
 *  parse_passwd () --
 *      take a struct password and fill in the fields of the
 *      struct finfo.
 */
static void parse_passwd(struct passwd *pw, struct finfo *pinfo)
{
    char *cp;

    if (pw) {
        pinfo->pw = pw;
        pinfo->username = pw->pw_name;
        /* use pw_gecos */
        cp = pw->pw_gecos;
        pinfo->full_name = cp;
        cp = strchr (cp, ',');
        if (cp) { *cp = 0, cp++; } else return;
        pinfo->office = cp;
        cp = strchr (cp, ',');
        if (cp) { *cp = 0, cp++; } else return;
        pinfo->office_phone = cp;
        cp = strchr (cp, ',');
        if (cp) { *cp = 0, cp++; } else return;
        pinfo->home_phone = cp;
        /*  extra fields contain site-specific information, and
         *  can not be changed by this version of chfn.  */
        cp = strchr (cp, ',');
        if (cp) { *cp = 0, cp++; } else return;
        pinfo->other = cp;
    }
}

/*
 *  ask_info () --
 *      prompt the user for the finger information and store it.
 */
static void ask_info(struct finfo *oldfp, struct finfo *newfp)
{
    printf ("Changing finger information for %s.\n", oldfp->username);
    newfp->full_name = prompt ("Name", oldfp->full_name);
    newfp->office = prompt ("Office", oldfp->office);
    newfp->office_phone = prompt ("Office Phone", oldfp->office_phone);
    newfp->home_phone = prompt ("Home Phone", oldfp->home_phone);
    printf ("\n");
}

/*
 *  prompt () --
 *      ask the user for a given field and check that the string is legal.
 */
static char *prompt(char *question, char *def_val)
{
    static char *blank = "none";
    int len;
    char *ans;
    char buf[1024];

    while (true) {
        if (! def_val) def_val = "";
        printf("%s [%s]: ", question, def_val);
        *buf = 0;
        if (fgets (buf, sizeof (buf), stdin) == NULL) {
            printf ("\nAborted.\n");
            exit (-1);
        }
        /* remove the newline at the end of buf. */
        ans = buf;
        while (isspace (*ans)) ans++;
        len = strlen (ans);
        while (len > 0 && isspace (ans[len-1])) len--;
        if (len <= 0) return NULL;
        ans[len] = 0;
        if (! strcasecmp (ans, blank)) return "";
        if (check_gecos_string (NULL, ans) >= 0) break;
    }
    return strdup(buf);
}

/*
 *  check_gecos_string () --
 *      check that the given gecos string is legal.  if it's not legal,
 *      output "msg" followed by a description of the problem, and
 *      return (-1).
 */
static int check_gecos_string(char *msg, char *gecos)
{
    int i, c;

    for (i = 0; i < strlen (gecos); i++) {
        c = gecos[i];
        if (c == ',' || c == ':' || c == '=' || c == '"') {
            if (msg) printf ("%s: ", msg);
            printf ("'%c' is not allowed.\n", c);
            return (-1);
        }
        if (iscntrl (c)) {
            if (msg) printf ("%s: ", msg);
            printf ("Control characters are not allowed.\n");
            return (-1);
        }
    }
    return (0);
}

/*
 *  set_changed_data () --
 *      incorporate the new data into the old finger info.
 */
static boolean set_changed_data(struct finfo *oldfp, struct finfo *newfp)
{
    boolean changed = false;

    if (newfp->full_name) {
        oldfp->full_name = newfp->full_name; changed = true;
    }
    if (newfp->office) {
        oldfp->office = newfp->office; changed = true;
    }
    if (newfp->office_phone) {
        oldfp->office_phone = newfp->office_phone; changed = true;
    }
    if (newfp->home_phone) {
        oldfp->home_phone = newfp->home_phone; changed = true;
    }
    return changed;
}

/*
 *  save_new_data () --
 *      save the given finger info in /etc/passwd.
 *      return zero on success.
 */
static int save_new_data(struct finfo *pinfo)
{
    char *gecos;
    int len;

    /* null fields will confuse printf(). */
    if (! pinfo->full_name) pinfo->full_name = "";
    if (! pinfo->office) pinfo->office = "";
    if (! pinfo->office_phone) pinfo->office_phone = "";
    if (! pinfo->home_phone) pinfo->home_phone = "";
    if (! pinfo->other) pinfo->other = "";

    /* create the new gecos string */
    len = (strlen (pinfo->full_name) + strlen (pinfo->office) +
           strlen (pinfo->office_phone) + strlen (pinfo->home_phone) +
           strlen (pinfo->other) + 4);
    gecos = malloc (len + 1);
    sprintf (gecos, "%s,%s,%s,%s,%s", pinfo->full_name, pinfo->office,
             pinfo->office_phone, pinfo->home_phone, pinfo->other);

    /* write the new struct passwd to the passwd file. */
    pinfo->pw->pw_gecos = gecos;
    if (setpwnam (pinfo->pw) < 0) {
        perror ("setpwnam");
        return (-1);
    }
    printf ("Finger information changed.\n");
    return 0;
}
