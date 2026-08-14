#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <unistd.h>
#include <sys/stat.h>
#include <dirent.h>
#include <time.h>
#include <grp.h>
#include <pwd.h>
#include <errno.h>

static int opt_all = 0;
static int opt_long = 0;
static int opt_human = 0;
static int opt_recursive = 0;
static int opt_time = 0;
static int opt_reverse = 0;

static void human_size(long size)
{
    static const char units[] = "kMGT";
    double d = (double)size;
    int u = -1;
    while (d >= 1024.0 && u < 3) {
        d /= 1024.0;
        u++;
    }
    if (u < 0)
        printf("%ld", size);
    else if (d >= 10)
        printf("%.0f%c", d, units[u]);
    else
        printf("%.1f%c", d, units[u]);
}

static char perm_char(int has_special, int has_exec, char special, char missing)
{
    if (has_special)
        return has_exec ? special : missing;
    return has_exec ? 'x' : '-';
}

static void print_perms(const struct stat *st)
{
    char t;
    if (S_ISREG(st->st_mode))
        t = '-';
    else if (S_ISDIR(st->st_mode))
        t = 'd';
    else if (S_ISLNK(st->st_mode))
        t = 'l';
    else if (S_ISCHR(st->st_mode))
        t = 'c';
    else if (S_ISBLK(st->st_mode))
        t = 'b';
    else if (S_ISFIFO(st->st_mode))
        t = 'p';
    else if (S_ISSOCK(st->st_mode))
        t = 's';
    else
        t = '?';

    printf("%c%c%c%c%c%c%c%c%c%c",
           t,
           (st->st_mode & S_IRUSR) ? 'r' : '-',
           (st->st_mode & S_IWUSR) ? 'w' : '-',
           perm_char(st->st_mode & S_ISUID, st->st_mode & S_IXUSR, 's', 'S'),
           (st->st_mode & S_IRGRP) ? 'r' : '-',
           (st->st_mode & S_IWGRP) ? 'w' : '-',
           perm_char(st->st_mode & S_ISGID, st->st_mode & S_IXGRP, 's', 'S'),
           (st->st_mode & S_IROTH) ? 'r' : '-',
           (st->st_mode & S_IWOTH) ? 'w' : '-',
           perm_char(st->st_mode & S_ISVTX, st->st_mode & S_IXOTH, 't', 'T'));
}

static void print_time(const struct stat *st)
{
    char buf[32];
    struct tm *tm = localtime(&st->st_mtime);

    if (!tm) {
        printf("???");
        return;
    }
    if (time(NULL) - st->st_mtime < 180 * 24 * 3600)
        strftime(buf, sizeof buf, "%b %e %H:%M", tm);
    else
        strftime(buf, sizeof buf, "%b %e  %Y", tm);
    printf("%s", buf);
}

static void print_long(const char *name, const struct stat *st)
{
    struct passwd *pw = getpwuid(st->st_uid);
    struct group *gr = getgrgid(st->st_gid);
    char owner[32], group[32];

    snprintf(owner, sizeof owner, "%s", pw ? pw->pw_name : "???");
    snprintf(group, sizeof group, "%s", gr ? gr->gr_name : "???");

    print_perms(st);
    printf(" %3ld %s %s ", st->st_nlink, owner, group);
    if (opt_human) {
        human_size(st->st_size);
        printf(" ");
    } else {
        printf("%7ld ", st->st_size);
    }
    print_time(st);
    printf(" %s\n", name);
}

static int is_dot(const char *name)
{
    return strcmp(name, ".") == 0 || strcmp(name, "..") == 0;
}

static char sort_dir[4096];

static int compare_mtime(const struct dirent **a, const struct dirent **b)
{
    char pa[sizeof sort_dir + 256], pb[sizeof sort_dir + 256];
    struct stat sa, sb;

    snprintf(pa, sizeof pa, "%s/%s", sort_dir, (*a)->d_name);
    snprintf(pb, sizeof pb, "%s/%s", sort_dir, (*b)->d_name);
    if (lstat(pa, &sa) != 0 || lstat(pb, &sb) != 0)
        return alphasort(a, b);
    if (sa.st_mtime > sb.st_mtime)
        return -1;
    if (sa.st_mtime < sb.st_mtime)
        return 1;
    return alphasort(a, b);
}

static int compare_entries(const struct dirent **a, const struct dirent **b)
{
    int cmp = opt_time ? compare_mtime(a, b) : alphasort(a, b);
    return opt_reverse ? -cmp : cmp;
}

static int list_dir(const char *path)
{
    DIR *d;
    struct dirent **entries;
    int n, i, had_error = 0;

    d = opendir(path);
    if (!d) {
        fprintf(stderr, "ls: cannot access '%s': %s\n", path, strerror(errno));
        return 1;
    }

    snprintf(sort_dir, sizeof sort_dir, "%s", path);
    n = scandir(path, &entries, NULL, compare_entries);
    if (n < 0) {
        fprintf(stderr, "ls: cannot read directory '%s': %s\n", path, strerror(errno));
        free(entries);
        closedir(d);
        return 1;
    }

    for (i = 0; i < n; i++) {
        struct dirent *e = entries[i];
        struct stat st;
        char full[4096];

        if (!opt_all && e->d_name[0] == '.' && !is_dot(e->d_name))
            continue;
        if (opt_recursive && is_dot(e->d_name))
            continue;

        snprintf(full, sizeof full, "%s/%s", path, e->d_name);
        if (lstat(full, &st) != 0) {
            fprintf(stderr, "ls: cannot access '%s': %s\n", full, strerror(errno));
            had_error = 1;
            continue;
        }

        if (opt_long)
            print_long(e->d_name, &st);
        else if (opt_recursive && S_ISDIR(st.st_mode))
            printf("%s/\n", e->d_name);
        else
            printf("%s\n", e->d_name);

        if (opt_recursive && S_ISDIR(st.st_mode) && !is_dot(e->d_name))
            had_error |= list_dir(full);
    }

    for (i = 0; i < n; i++)
        free(entries[i]);
    free(entries);
    closedir(d);
    return had_error;
}

static void usage(void)
{
    fprintf(stderr,
            "usage: ls [-lahR] [file ...]\n"
            "  -a  show all files, including names starting with .\n"
            "  -l  use a long listing format\n"
            "  -h  print sizes in human readable format (with -l)\n"
            "  -R  list subdirectories recursively\n"
            "  -t  sort by modification time, newest first\n"
            "  -r  reverse order\n");
    exit(2);
}

int main(int argc, char **argv)
{
    int i, opt, rc = 0;
    char *paths[256];
    int np = 0;

    while ((opt = getopt(argc, argv, "lahRtr")) != -1) {
        switch (opt) {
        case 'a':
            opt_all = 1;
            break;
        case 'l':
            opt_long = 1;
            break;
        case 'h':
            opt_human = 1;
            break;
        case 'R':
            opt_recursive = 1;
            break;
        case 't':
            opt_time = 1;
            break;
        case 'r':
            opt_reverse = 1;
            break;
        default:
            usage();
        }
    }

    for (i = optind; i < argc; i++) {
        if (np >= 255) {
            fprintf(stderr, "ls: too many arguments\n");
            return 2;
        }
        paths[np++] = argv[i];
    }
    if (np == 0)
        paths[np++] = ".";

    if (np > 1) {
        for (i = 0; i < np; i++) {
            struct stat st;

            if (i > 0)
                printf("\n");
            if (np > 1)
                printf("%s:\n", paths[i]);
            if (lstat(paths[i], &st) != 0) {
                fprintf(stderr, "ls: cannot access '%s': %s\n", paths[i], strerror(errno));
                rc = 1;
                continue;
            }
            if (S_ISDIR(st.st_mode))
                rc |= list_dir(paths[i]);
            else {
                if (opt_long)
                    print_long(paths[i], &st);
                else
                    printf("%s\n", paths[i]);
            }
        }
    } else {
        struct stat st;

        if (lstat(paths[0], &st) != 0) {
            fprintf(stderr, "ls: cannot access '%s': %s\n", paths[0], strerror(errno));
            return 1;
        }
        if (S_ISDIR(st.st_mode))
            rc = list_dir(paths[0]);
        else {
            if (opt_long)
                print_long(paths[0], &st);
            else
                printf("%s\n", paths[0]);
        }
    }

    return rc;
}
