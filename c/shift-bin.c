//
// Copyright (C) 2012-2019 United States Government as represented by the
// Administrator of the National Aeronautics and Space Administration
// (NASA).  All Rights Reserved.
//
// This software is distributed under the NASA Open Source Agreement
// (NOSA), version 1.3.  The NOSA has been approved by the Open Source
// Initiative.  See http://www.opensource.org/licenses/nasa1.3.php
// for the complete NOSA document.
//
// THE SUBJECT SOFTWARE IS PROVIDED "AS IS" WITHOUT ANY WARRANTY OF ANY
// KIND, EITHER EXPRESSED, IMPLIED, OR STATUTORY, INCLUDING, BUT NOT
// LIMITED TO, ANY WARRANTY THAT THE SUBJECT SOFTWARE WILL CONFORM TO
// SPECIFICATIONS, ANY IMPLIED WARRANTIES OF MERCHANTABILITY, FITNESS FOR
// A PARTICULAR PURPOSE, OR FREEDOM FROM INFRINGEMENT, ANY WARRANTY THAT
// THE SUBJECT SOFTWARE WILL BE ERROR FREE, OR ANY WARRANTY THAT
// DOCUMENTATION, IF PROVIDED, WILL CONFORM TO THE SUBJECT SOFTWARE. THIS
// AGREEMENT DOES NOT, IN ANY MANNER, CONSTITUTE AN ENDORSEMENT BY
// GOVERNMENT AGENCY OR ANY PRIOR RECIPIENT OF ANY RESULTS, RESULTING
// DESIGNS, HARDWARE, SOFTWARE PRODUCTS OR ANY OTHER APPLICATIONS RESULTING
// FROM USE OF THE SUBJECT SOFTWARE.  FURTHER, GOVERNMENT AGENCY DISCLAIMS
// ALL WARRANTIES AND LIABILITIES REGARDING THIRD-PARTY SOFTWARE, IF
// PRESENT IN THE ORIGINAL SOFTWARE, AND DISTRIBUTES IT "AS IS".
//
// RECIPIENT AGREES TO WAIVE ANY AND ALL CLAIMS AGAINST THE UNITED STATES
// GOVERNMENT, ITS CONTRACTORS AND SUBCONTRACTORS, AS WELL AS ANY PRIOR
// RECIPIENT.  IF RECIPIENT'S USE OF THE SUBJECT SOFTWARE RESULTS IN ANY
// LIABILITIES, DEMANDS, DAMAGES, EXPENSES OR LOSSES ARISING FROM SUCH USE,
// INCLUDING ANY DAMAGES FROM PRODUCTS BASED ON, OR RESULTING FROM,
// RECIPIENT'S USE OF THE SUBJECT SOFTWARE, RECIPIENT SHALL INDEMNIFY AND
// HOLD HARMLESS THE UNITED STATES GOVERNMENT, ITS CONTRACTORS AND
// SUBCONTRACTORS, AS WELL AS ANY PRIOR RECIPIENT, TO THE EXTENT PERMITTED
// BY LAW.  RECIPIENT'S SOLE REMEDY FOR ANY SUCH MATTER SHALL BE THE
// IMMEDIATE, UNILATERAL TERMINATION OF THIS AGREEMENT.
//

#define _GNU_SOURCE
#include <fcntl.h>
#include <grp.h>
#include <pwd.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#ifndef _NO_LUSTRE
# include <lustre/lustreapi.h>
#endif
#include <sys/acl.h>
#include <sys/time.h>
#include <sys/types.h>
#include <sys/xattr.h>

////////////////
//// escape ////
////////////////
// adapted from http://www.geekhideout.com/urlcode.shtml
char *escape(char *str) {
    if (str == NULL) return str;
    static char hex[] = "0123456789ABCDEF";
    char *pstr = str;
    char *buf = malloc(strlen(str) * 3 + 1);
    char *pbuf = buf;
    while (*pstr) {
        if (isalnum(*pstr) || *pstr == '-' || *pstr == '_' ||
                *pstr == '.' || *pstr == '~') {
            *pbuf++ = *pstr;
        } else {
            *pbuf++ = '%';
            *pbuf++ = hex[(*pstr >> 4) & 15];
            *pbuf++ = hex[*pstr & 15];
        }
        pstr++;
    }
    *pbuf = '\0';
    return buf;
}

//////////////////
//// unescape ////
//////////////////
// adapted from http://www.geekhideout.com/urlcode.shtml
char *unescape(char *str) {
    if (str == NULL) return str;
    char *pstr = str;
    char *buf = malloc(strlen(str) + 1);
    char *pbuf = buf;
    while (*pstr) {
        if (*pstr == '%') {
            if (pstr[1] && pstr[2]) {
                *pbuf = (isdigit(pstr[1]) ? pstr[1] - '0' :
                    tolower(pstr[1]) - 'a' + 10) << 4;
                *pbuf++ |= isdigit(pstr[2]) ? pstr[2] - '0' :
                    tolower(pstr[2]) - 'a' + 10;
                pstr += 2;
            }
        } else {
            *pbuf++ = *pstr;
        }
        pstr++;
    }
    *pbuf = '\0';
    return buf;
}

//////////////////
//// do_chown ////
//////////////////
int do_chown(char *file, char *user, char *group) {
    struct passwd *pwd = getpwnam(user);
    if (pwd == NULL) return -1;
    uid_t uid = pwd->pw_uid;
    struct group *grp = getgrnam(group);
    if (grp == NULL) return -1;
    gid_t gid = grp->gr_gid;
    return lchown(file, uid, gid);
}

////////////////////
//// do_fadvise ////
////////////////////
int do_fadvise(char *file, off_t off, off_t len) {
    int fd = open(file, O_RDONLY);
    if (fd == -1) return fd;
    return posix_fadvise(fd, off, len, POSIX_FADV_DONTNEED);
}

//////////////////////
//// do_fallocate ////
//////////////////////
int do_fallocate(char *file, off_t len) {
    int fd = open(file, O_CREAT | O_WRONLY);
    if (fd == -1) return fd;
    int rc = fallocate(fd, FALLOC_FL_KEEP_SIZE, 0, len);
    close(fd);
    return rc;
}

////////////////////
//// do_getfacl ////
////////////////////
int do_getfacl(char *file) {
    acl_t acl = acl_get_file(file, ACL_TYPE_ACCESS);
    if (acl == NULL || acl_equiv_mode(acl, NULL) == 0) return -1;
    char *s = acl_to_text(acl, NULL);
    if (s == NULL) return -1;
    char *es = escape(s);
    printf(" %s", es);
    free(es);
    acl_free(s);
    return 0;
}

////////////////////
//// do_setfacl ////
////////////////////
int do_setfacl(char *file, char *s_acl) {
    acl_t acl = acl_from_text(s_acl);
    if (acl == NULL) return -1;
    int rc = acl_set_file(file, ACL_TYPE_ACCESS, acl);
    acl_free(acl);
    return rc;
}

/////////////////////
//// do_getfattr ////
/////////////////////
int do_getfattr(char *file) {
    ssize_t buflen, keylen, vallen;
    char *buf, *key, *val;
    buflen = listxattr(file, NULL, 0);
    if (buflen <= 0) return -1;
    buf = malloc(buflen);
    if (buf == NULL) return -1;
    buflen = listxattr(file, buf, buflen);
    if (buflen == -1) return -1;

    int count = 0;
    key = buf;
    while (buflen > 0) {
        if (!strncmp(key, "user.", 5)) {
            vallen = getxattr(file, key, NULL, 0);
            if (vallen > 0) {
                val = malloc(vallen + 1);
                if (val != NULL) {
                    vallen = getxattr(file, key, val, vallen);
                    if (vallen != -1) {
                        val[vallen] = 0;
                        char *eval = escape(val);
                        printf("%s", count == 0 ? " " : ",");
                        printf("%s\%3D%s", key, eval);
                        free(eval);
                        count++;
                    }
                }
                free(val);
            }
        }
        keylen = strlen(key) + 1;
        buflen -= keylen;
        key += keylen;
    }
    free(buf);
    return count ? 0 : -1;
}

/////////////////////
//// do_setfattr ////
/////////////////////
int do_setfattr(char *file, char *s_xattr) {
    int rc = 0;
    char *keyval = strtok(s_xattr, ",");
    while (keyval != NULL) {
        char *key = unescape(keyval);
        char *val = index(key, '=');
        if (val == NULL) continue;
        *val++ = '\0';
        rc |= lsetxattr(file, key, val, strlen(val), 0);
        free(key);
        keyval = strtok(NULL, ",");
    }
    return rc;
}

//////////////////////
//// do_getstripe ////
//////////////////////
int do_getstripe(char *file) {
#ifndef _NO_LUSTRE
    int v1 = sizeof(struct lov_user_md_v1) +
        LOV_MAX_STRIPE_COUNT * sizeof(struct lov_user_ost_data_v1);
    int v3 = sizeof(struct lov_user_md_v3) +
        LOV_MAX_STRIPE_COUNT * sizeof(struct lov_user_ost_data_v1);
    struct lov_user_md *lum = malloc(v1 > v3 ? v1 : v3);
    if (lum == NULL) return -1;
    int rc = llapi_file_get_stripe(file, lum);
    if (!rc) printf(" %d,%d", lum->lmm_stripe_count, lum->lmm_stripe_size);
    free(lum);
    return rc;
#else
    return -1;
#endif
}

//////////////////////
//// do_setstripe ////
//////////////////////
int do_setstripe(char *file, int scount, unsigned long long ssize, char *pool) {
#ifndef _NO_LUSTRE
    return llapi_file_create_pool(file, ssize, -1, scount, 0, pool);
#else
    return -1;
#endif
}

//////////////////
//// do_utime ////
//////////////////
int do_utime(char *file, long atime, long mtime) {
    struct timeval tv[2];
    tv[0].tv_sec = atime;
    tv[0].tv_usec = 0;
    tv[1].tv_sec = mtime;
    tv[1].tv_usec = 0;
    return lutimes(file, tv);
}

//////////////
//// main ////
//////////////
int main(int argc, char *argv[]) {
    char *buf = NULL;
    size_t buf_max = 0;

    while (getline(&buf, &buf_max, stdin) > 0) {
        buf[strcspn(buf, "\n")] = '\0';
        char *cmd = strtok(buf, " ");
        if (cmd == NULL) continue;
        char *efile = strtok(NULL, " ");
        char *file = unescape(efile);
        if (file == NULL) continue;
        char *a1, *a2, *a3;
        a1 = strtok(NULL, " ");
        a2 = strtok(NULL, " ");
        a3 = strtok(NULL, " ");
        printf("%s %s", cmd, efile);
        int rc = -1;
        if (!strcmp(cmd, "chown") && a1 != NULL && a2 != NULL) {
            rc = do_chown(file, a1, a2);
        } else if (!strcmp(cmd, "fadvise") && a1 != NULL && a2 != NULL) {
            rc = do_fadvise(file, atoll(a1), atoll(a2));
        } else if (!strcmp(cmd, "fallocate") && a1 != NULL) {
            rc = do_fallocate(file, atoll(a1));
        } else if (!strcmp(cmd, "getfacl")) {
            rc = do_getfacl(file);
        } else if (!strcmp(cmd, "setfacl") && a1 != NULL) {
            char *ua1 = unescape(a1);
            rc = do_setfacl(file, ua1);
            free(ua1);
        } else if (!strcmp(cmd, "getstripe")) {
            rc = do_getstripe(file);
        } else if (!strcmp(cmd, "setstripe") && a1 != NULL && a2 != NULL) {
            rc = do_setstripe(file, atol(a1), atoll(a2), a3);
        } else if (!strcmp(cmd, "getfattr")) {
            rc = do_getfattr(file);
        } else if (!strcmp(cmd, "setfattr")) {
            char *ua1 = unescape(a1);
            rc = do_setfattr(file, ua1);
            free(ua1);
        } else if (!strcmp(cmd, "utime") && a1 != NULL && a2 != NULL) {
            rc = do_utime(file, atol(a1), atol(a2));
        }
        printf(" %d\n", rc);
        fflush(stdout);
        free(file);
    }
    free(buf);
}

