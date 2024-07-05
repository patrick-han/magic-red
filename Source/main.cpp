#include <Renderer.h>

int main() {
    Renderer renderer;

    try {
        renderer.run();
    } catch (const std::exception& e) {
        std::cerr << "[EXCEPTION] " << e.what() << std::endl;
        return EXIT_FAILURE;
    }

    return EXIT_SUCCESS;
}
