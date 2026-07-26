#include "task/debug_task.h"
#include "common/print.h"
#include "control/heading/heading.h"

namespace task::debug {
namespace {
#define KV_INT(k,v) do{common::uart_print(k"=");common::uart_print_int(v);common::uart_print(",");}while(0)
#define KV_F1(k,v)  do{common::uart_print(k"=");common::uart_print_f1(v);common::uart_print(",");}while(0)
}
void init() {}
void loop(const sensors::state::State& s) {
    KV_F1("roll",s.roll); KV_F1("pitch",s.pitch); KV_F1("yaw",s.yaw);
    KV_F1("yaw_rate",s.yaw_rate); KV_F1("hd_err",control::heading::last_error());
    KV_INT("m1",s.m1); KV_INT("m2",s.m2); KV_INT("m3",s.m3); KV_INT("m4",s.m4);
    KV_INT("v.valid",s.vision.valid); KV_INT("v.fresh",s.vision.fresh);
    KV_INT("v.flag",s.vision.flag); KV_INT("v.type",s.vision.type);
    KV_INT("v.hb",s.vision.heartbeat_cnt);
    KV_INT("v.x",(int32_t)s.vision.x); KV_INT("v.y",(int32_t)s.vision.y);
    KV_INT("v.bx",s.vision.bad_xor); KV_INT("v.ovf",s.vision.overflow);
    common::uart_println();
}
#undef KV_INT
#undef KV_F1
}