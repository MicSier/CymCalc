#define NOB_IMPLEMENTATION
#include "nob.h"

int main(int argc, char **argv) {
    NOB_GO_REBUILD_URSELF(argc, argv);

    const char *src_file = "examples\\examples.c";
    const char *exe_file = "prog.exe";
    const char *regression_file = "examples\\example.regression.txt";
    const char *regression_file_err = "examples\\example.regression.err.txt";
    const char *output_file = "examples\\example.output.txt";
    const char *output_file_err ="examples\\example.output.err.txt";

    Nob_Cmd cmd = {0};
    Nob_Fd fdout = NOB_INVALID_FD;

    if (argc < 2) {
        nob_log(NOB_ERROR, "Usage: nob <build|run|record|test>");
        return 1;
    }

    const char *mode = argv[1];

    // Compile
    nob_cmd_append(&cmd,
        "gcc", "-g", "-w", src_file,
        "-I", "C:/vcpkg/installed/x64-mingw-static/include",
        "-L", "C:/vcpkg/installed/x64-mingw-static/lib",
        "-lgmp", "-static",
        "-o", exe_file
    );
    if (!nob_cmd_run_sync_and_reset(&cmd)) return 1;
    if (strcmp(mode, "build") == 0) {
        nob_log(NOB_INFO, "Build Sucessful.");
        return 0;
    } 

    // Run
    if (strcmp(mode, "run") == 0) {
        nob_cmd_append(&cmd, exe_file);
        if (!nob_cmd_run_sync_and_reset(&cmd)) return 1;
        return 0;
    }

    // Record (rebase)
    if (strcmp(mode, "record") == 0) {
        //Nob_Fd fdin = nob_fd_open_for_read(input_file);
        //if (fdin == NOB_INVALID_FD) fail();
        Nob_Fd fdout = nob_fd_open_for_write(regression_file);
        if (fdout == NOB_INVALID_FD) return 1;
        Nob_Fd fderr = nob_fd_open_for_write(regression_file_err);
        if (fderr == NOB_INVALID_FD) return 1;
        Nob_String_Builder sb = {0};
        nob_sb_appendf(&sb, "%s", exe_file);
        nob_cmd_append(&cmd, sb.items);

        if (!nob_cmd_run_sync_redirect_and_reset(&cmd, (Nob_Cmd_Redirect) {
            //.fdin = &fdin,
            .fdout = &fdout,
            .fderr = &fderr
        })) return 1;

       // if (!nob_cmd_run_sync_and_reset(&cmd)) return 1;
        nob_sb_free(sb);
        return 0;
    }

    // Test
    if (strcmp(mode, "test") == 0) {
        Nob_Fd fdout = nob_fd_open_for_write(output_file);
        if (fdout == NOB_INVALID_FD) return 1;

        Nob_Fd fderr = nob_fd_open_for_write(output_file_err);
        if (fderr == NOB_INVALID_FD) return 1;

        Nob_String_Builder sb = {0};
        nob_sb_appendf(&sb, "%s", exe_file);
        nob_cmd_append(&cmd, sb.items);

        if (!nob_cmd_run_sync_redirect_and_reset(&cmd, (Nob_Cmd_Redirect) {
            .fdout = &fdout,
            .fderr = &fderr
        })) return 1;

        nob_sb_free(sb);

        // Diff (using Windows fc)
        nob_cmd_append(&cmd, "fc", output_file, regression_file);
        bool ok = nob_cmd_run_sync_and_reset(&cmd);
        if (!ok) {
            nob_log(NOB_ERROR, "Regression test failed. Differences found.");
            return 1;
        } else {
            nob_log(NOB_INFO, "Test passed. No differences.");
            return 0;
        }
    }

    nob_log(NOB_ERROR, "Unknown mode: %s", mode);
    return 1;
}