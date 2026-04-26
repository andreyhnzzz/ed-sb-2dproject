#include <raylib.h>

#include "core/application/ApplicationSession.h"

int main(int argc, char* argv[]) {
    ApplicationSession app;
    if (!app.initialize(argc, argv)) {
        return 1;
    }

    return app.run();
}
