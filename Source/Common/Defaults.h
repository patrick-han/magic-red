#pragma once
#include "vulkan/vulkan.h"
constexpr VkClearValue DEFAULT_CLEAR_VALUE_COLOR = {{0.5f, 0.5f, 0.7f, 1.0f}};
constexpr VkClearValue DEFAULT_CLEAR_VALUE_DEPTH = {{1.0f, 0}};
constexpr VkViewport DEFAULT_VIEWPORT_FULLSCREEN = { 0.0f, 0.0f, static_cast<float>(WINDOW_WIDTH), static_cast<float>(WINDOW_HEIGHT), 0.0f, 1.0f };
constexpr VkRect2D DEFAULT_SCISSOR_FULLSCREEN = { {0, 0}, {WINDOW_WIDTH, WINDOW_HEIGHT}};