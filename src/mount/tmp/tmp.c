/* 
 * Copyright (c) 2015-2016, Gregory M. Kurtzer. All rights reserved.
 * 
 * “Singularity” Copyright (c) 2016, The Regents of the University of California,
 * through Lawrence Berkeley National Laboratory (subject to receipt of any
 * required approvals from the U.S. Dept. of Energy).  All rights reserved.
 * 
 * This software is licensed under a customized 3-clause BSD license.  Please
 * consult LICENSE file distributed with the sources of this project regarding
 * your rights to use or distribute this software.
 * 
 * NOTICE.  This Software was developed under funding from the U.S. Department of
 * Energy and the U.S. Government consequently retains certain rights. As such,
 * the U.S. Government has been granted for itself and others acting on its
 * behalf a paid-up, nonexclusive, irrevocable, worldwide license in the Software
 * to reproduce, distribute copies to the public, prepare derivative works, and
 * perform publicly and display publicly, and to permit other to do so. 
 * 
*/

#include <errno.h>
#include <fcntl.h>
#include <stdio.h>
#include <string.h>
#include <sys/mount.h>
#include <sys/stat.h>
#include <sys/types.h>
#include <unistd.h>
#include <stdlib.h>
#include <pwd.h>

#include "file.h"
#include "util.h"
#include "message.h"
#include "privilege.h"
#include "config_parser.h"
#include "sessiondir.h"
#include "rootfs/rootfs.h"


int singularity_mount_tmp(void) {
    char *container_dir = singularity_rootfs_dir();
    char *tmp_source;
    char *vartmp_source;
    char *scratchpath;

    config_rewind();
    if ( config_get_key_bool("mount tmp", 1) <= 0 ) {
        message(VERBOSE, "Skipping tmp dir mounting (per config)\n");
        return(0);
    }

    config_rewind();
    if ( ( config_get_key_bool("user bind control", 1) > 0 ) && ( ( scratchpath = getenv("SINGULARITY_SCRATCH") ) != NULL ) ) {
        tmp_source = joinpath(scratchpath, "/tmp");
        vartmp_source = joinpath(scratchpath, "/var_tmp");
    } else if ( getenv("SINGULARITY_CONTAIN") != NULL ) {
        char *sessiondir = singularity_sessiondir_get();
        tmp_source = joinpath(sessiondir, "/tmp");
        vartmp_source = joinpath(sessiondir, "/var_tmp");
    } else {
        tmp_source = strdup("/tmp");
        vartmp_source = strdup("/var/tmp");
    }

    if ( s_mkpath(tmp_source, 0755) < 0 ) {
        message(ABRT, "Could not create tmp directory %s: %s\n", tmp_source, strerror(errno));
        ABORT(255);
    }
    if ( s_mkpath(vartmp_source, 0755) < 0 ) {
        message(ABRT, "Could not create vartmp directory %s: %s\n", vartmp_source, strerror(errno));
        ABORT(255);
    }

    if ( is_dir(tmp_source) == 0 ) {
        if ( is_dir(joinpath(container_dir, "/tmp")) == 0 ) {
            priv_escalate();
            message(VERBOSE, "Mounting directory: /tmp\n");
            if ( mount(tmp_source, joinpath(container_dir, "/tmp"), NULL, MS_BIND|MS_NOSUID|MS_REC, NULL) < 0 ) {
                message(ERROR, "Failed to mount %s -> /tmp: %s\n", tmp_source, strerror(errno));
                ABORT(255);
            }
            priv_drop();
        } else {
            message(VERBOSE, "Could not mount container's /tmp directory: does not exist\n");
        }
    } else {
        message(VERBOSE, "Could not mount host's /tmp directory (%s): does not exist\n", tmp_source);
    }

    if ( is_dir(vartmp_source) == 0 ) {
        if ( is_dir(joinpath(container_dir, "/var/tmp")) == 0 ) {
            priv_escalate();
            message(VERBOSE, "Mounting directory: /var/tmp\n");
            if ( mount(vartmp_source, joinpath(container_dir, "/var/tmp"), NULL, MS_BIND|MS_NOSUID|MS_REC, NULL) < 0 ) {
                message(ERROR, "Failed to mount %s -> /var/tmp: %s\n", vartmp_source, strerror(errno));
                ABORT(255);
            }
            priv_drop();
        } else {
            message(VERBOSE, "Could not mount container's /var/tmp directory: does not exist\n");
        }
    } else {
        message(VERBOSE, "Could not mount host's /var/tmp directory (%s): does not exist\n", vartmp_source);
    }

    free(tmp_source);
    free(vartmp_source);

    return(0);
}