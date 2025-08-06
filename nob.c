#define NOB_IMPLEMENTATION
#include "nob.h"

#define REGRESSION_COUNT 4
int main(int argc, char **argv) {
    NOB_GO_REBUILD_URSELF(argc, argv);

    const char *extension_src = ".c";
    const char *extension_exec = ".exe";

    const char *regression_path = "examples\\";
    const char *output_path = "examples\\";
    
    const char *regression_files[REGRESSION_COUNT] = {"number_arithmetic","symbol_and_number_arithmetic","calculus","evaluation"};
    const char *regression_str = ".regression";
    const char *output_str = ".output";
    const char *err_str = ".err";
    const char *extension_data = ".txt";

    Nob_Cmd cmd = {0};
    Nob_Fd fdout = NOB_INVALID_FD;

    if (argc < 2) {
        nob_log(NOB_ERROR, "Usage: nob <build|run|record|test>");
        return 1;
    }

    const char *mode = argv[1];

    // Compile
    for(size_t i =0; i<REGRESSION_COUNT;i++) {
        char src_file[256];
        sprintf(src_file, "%s%s%s",regression_path,regression_files[i],extension_src);
        char exec_file[256];
        sprintf(exec_file, "%s%s%s",regression_path,regression_files[i],extension_exec);
        nob_cmd_append(&cmd,
            "gcc", "-g", "-w", src_file,
            "-I", "C:/vcpkg/installed/x64-mingw-static/include",
            "-L", "C:/vcpkg/installed/x64-mingw-static/lib",
            "-lgmp", "-static",
            "-o", exec_file
        );

        if (!nob_cmd_run_sync_and_reset(&cmd)) return 1;
        nob_log(NOB_INFO, "Build Sucessful: %s", src_file);
    }
    if (strcmp(mode, "build") == 0) return 0;

    // Run
    if (strcmp(mode, "run") == 0) {
        for(size_t i =0; i<REGRESSION_COUNT;i++) {
            char exec_file[256];
            sprintf(exec_file, "%s%s%s",regression_path,regression_files[i],extension_exec);
            nob_log(NOB_INFO, "Build Running: %s", exec_file);
            nob_cmd_append(&cmd, exec_file);
            if (!nob_cmd_run_sync_and_reset(&cmd)) return 1;
        }
        return 0;
    }

    // Record (rebase)
    if (strcmp(mode, "record") == 0) {
        //Nob_Fd fdin = nob_fd_open_for_read(input_file);
        //if (fdin == NOB_INVALID_FD) fail();

        for(size_t i =0; i<REGRESSION_COUNT;i++) {
            nob_log(NOB_INFO, "Recording %s", regression_files[i]);
            char regression_file[256];
            sprintf(regression_file, "%s%s%s%s",regression_path,regression_files[i],regression_str,extension_data);
            Nob_Fd fdout = nob_fd_open_for_write(regression_file);
            if (fdout == NOB_INVALID_FD) return 1;
            char regression_file_err[256];
            sprintf(regression_file_err, "%s%s%s%s%s",regression_path,regression_files[i],regression_str,err_str,extension_data);
            Nob_Fd fderr = nob_fd_open_for_write(regression_file_err);
            if (fderr == NOB_INVALID_FD) return 1;
            Nob_String_Builder sb = {0};
            char exec_file[256];
            sprintf(exec_file, "%s%s%s",regression_path,regression_files[i],extension_exec);
            nob_sb_appendf(&sb, "%s", exec_file);
            nob_cmd_append(&cmd, sb.items);

            if (!nob_cmd_run_sync_redirect_and_reset(&cmd, (Nob_Cmd_Redirect) {
                //.fdin = &fdin,
                .fdout = &fdout,
                .fderr = &fderr
            })) return 1;

            nob_sb_free(sb);
        }
        
        return 0;
    }

    // Test
    if (strcmp(mode, "test") == 0) {
        for(size_t i = 0 ; i<REGRESSION_COUNT;i++) {
            nob_log(NOB_INFO, "Testing %s", regression_files[i]);
            char output_file[256];
            sprintf(output_file, "%s%s%s%s",regression_path,regression_files[i],output_str,extension_data);
            
            Nob_Fd fdout = nob_fd_open_for_write(output_file);
            if (fdout == NOB_INVALID_FD) return 1;
            char output_file_err[256];
            sprintf(output_file_err, "%s%s%s%s%s",regression_path,regression_files[i],output_str,err_str,extension_data);


            Nob_Fd fderr = nob_fd_open_for_write(output_file_err);
            if (fderr == NOB_INVALID_FD) return 1;

            Nob_String_Builder sb = {0};
            char exec_file[256];
            sprintf(exec_file, "%s%s%s",regression_path,regression_files[i],extension_exec);
            nob_sb_appendf(&sb, "%s", exec_file);
            nob_cmd_append(&cmd, sb.items);

            if (!nob_cmd_run_sync_redirect_and_reset(&cmd, (Nob_Cmd_Redirect) {
                .fdout = &fdout,
                .fderr = &fderr
            })) return 1;

            nob_sb_free(sb);

            // Diff (using Windows fc)
            char regression_file[256];
            sprintf(regression_file, "%s%s%s%s",regression_path,regression_files[i],regression_str,extension_data);

            nob_cmd_append(&cmd, "fc", output_file, regression_file);
            bool ok = nob_cmd_run_sync_and_reset(&cmd);
            if (!ok) {
                nob_log(NOB_ERROR, "Regression test failed. Differences found.");
                return 1;
            } else {
                nob_log(NOB_INFO, "Test passed. No differences.");
            }
        }
            nob_log(NOB_INFO, "ALL tests passed. No differences.");
            return 0;
    }

    nob_log(NOB_ERROR, "Unknown mode: %s", mode);
    return 1;
}