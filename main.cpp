#include "ti_msp_dl_config.h"
#include "app/app.h"

int main(void) {
    SYSCFG_DL_init();
    app::init();
    while (1) {
        app::tick();
    }
}