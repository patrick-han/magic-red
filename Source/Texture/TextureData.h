#pragma once
struct TextureSize {
    int x{-1};
    int y{-1};
    int ch{-1};
};

struct TextureLoadingData {
    void *data{nullptr};
    TextureSize texSize;
};
