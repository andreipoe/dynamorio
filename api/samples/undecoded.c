/* ******************************************************************************
 * Copyright (c) 2015-2018 Google, Inc.  All rights reserved.
 * ******************************************************************************/

/*
 * Redistribution and use in source and binary forms, with or without
 * modification, are permitted provided that the following conditions are met:
 *
 * * Redistributions of source code must retain the above copyright notice,
 *   this list of conditions and the following disclaimer.
 *
 * * Redistributions in binary form must reproduce the above copyright notice,
 *   this list of conditions and the following disclaimer in the documentation
 *   and/or other materials provided with the distribution.
 *
 * * Neither the name of VMware, Inc. nor the names of its contributors may be
 *   used to endorse or promote products derived from this software without
 *   specific prior written permission.
 *
 * THIS SOFTWARE IS PROVIDED BY THE COPYRIGHT HOLDERS AND CONTRIBUTORS "AS IS"
 * AND ANY EXPRESS OR IMPLIED WARRANTIES, INCLUDING, BUT NOT LIMITED TO, THE
 * IMPLIED WARRANTIES OF MERCHANTABILITY AND FITNESS FOR A PARTICULAR PURPOSE
 * ARE DISCLAIMED. IN NO EVENT SHALL VMWARE, INC. OR CONTRIBUTORS BE LIABLE
 * FOR ANY DIRECT, INDIRECT, INCIDENTAL, SPECIAL, EXEMPLARY, OR CONSEQUENTIAL
 * DAMAGES (INCLUDING, BUT NOT LIMITED TO, PROCUREMENT OF SUBSTITUTE GOODS OR
 * SERVICES; LOSS OF USE, DATA, OR PROFITS; OR BUSINESS INTERRUPTION) HOWEVER
 * CAUSED AND ON ANY THEORY OF LIABILITY, WHETHER IN CONTRACT, STRICT
 * LIABILITY, OR TORT (INCLUDING NEGLIGENCE OR OTHERWISE) ARISING IN ANY WAY
 * OUT OF THE USE OF THIS SOFTWARE, EVEN IF ADVISED OF THE POSSIBILITY OF SUCH
 * DAMAGE.
 */

/* Prints a string of hex-encoded bytes for each undecoded instruction.
 * Output can be piped to llvm-mc to be decoded.
 */

#include "dr_api.h"
#include "drmgr.h"
#include "drx.h"

#ifdef WINDOWS
#    define DISPLAY_STRING(msg) dr_messagebox(msg)
#else
#    define DISPLAY_STRING(msg) dr_printf("%s\n", msg);
#endif

#define NULL_TERMINATE(buf) buf[(sizeof(buf) / sizeof(buf[0])) - 1] = '\0'

#define SHOW_RESULTS
#define FNAME "undecoded.txt"
static uint undecoded = 0;
static FILE* outf;

static void
event_exit(void);
static dr_emit_flags_t
event_app_instruction(void *drcontext, void *tag, instrlist_t *bb, instr_t *instr,
                      bool for_trace, bool translating, void *user_data);

DR_EXPORT void
dr_client_main(client_id_t id, int argc, const char *argv[])
{
#ifndef AARCH64
    // Only aarch64 is supported
    return;
#else
    dr_set_client_name("DynamoRIO Sample Client 'undecoded'",
                       "http://dynamorio.org/issues");
    if (!drmgr_init())
        DR_ASSERT(false);
    drx_init();

    outf = (FILE*)fopen("undecoded.txt", "w");

    /* Register events: */
    dr_register_exit_event(event_exit);
    if (!drmgr_register_bb_instrumentation_event(NULL, event_app_instruction, NULL))
        DR_ASSERT(false);



    /* Make it easy to tell from the log file which client executed. */
    dr_log(NULL, DR_LOG_ALL, 1, "Client 'undecoded' initializing\n");
#    ifdef SHOW_RESULTS
    /* Also give notification to stderr. */
    if (dr_is_notify_on()) {
#        ifdef WINDOWS
        /* Ask for best-effort printing to cmd window.  Must be called at init. */
        dr_enable_console_printing();
#        endif
        dr_fprintf(STDERR, "Client undecoded is running\n");
    }
#    endif /* SHOW_RESULTS */
#endif /* AARCH64 */
}

static void
event_exit(void)
{
#ifdef SHOW_RESULTS
    dr_printf("%u undecoded instructions collected.\n", undecoded);
#endif
    if (!drmgr_unregister_bb_insertion_event(event_app_instruction))
        DR_ASSERT(false);
    drx_exit();
    drmgr_exit();

    fclose(outf);
}

/* This is called separately for each instruction in the block. */
static dr_emit_flags_t
event_app_instruction(void *drcontext, void *tag, instrlist_t *bb, instr_t *instr,
                      bool for_trace, bool translating, void *user_data)
{
    drmgr_disable_auto_predication(drcontext, bb);
    if (drmgr_is_first_instr(drcontext, instr)) {

        /* Normally looking ahead should be performed in the analysis event, but
         * here that would require storing the counts into an array passed in
         * user_data.  We avoid that overhead by cheating drmgr's model a little
         * bit and looking forward.  An alternative approach would be to insert
         * each counter before its respective instruction and have an
         * instru2instru pass that pulls the increments together to reduce
         * overhead.
         */
        for (instr_t* ins = instrlist_first_app(bb); ins != NULL; ins = instr_get_next_app(ins)) {
            int opcode = instr_get_opcode(ins);
            if (opcode == OP_xx)
            {
                byte* pc = instr_get_raw_bits(ins);
                // uint enc = *(uint *)pc;

                for (int b = 0; b < 4; b++)
                    fprintf(outf, "0x%02x ", pc[b]);
                fprintf(outf, "\n");

                undecoded++;
            }
        }
    }
    return DR_EMIT_DEFAULT;
}
