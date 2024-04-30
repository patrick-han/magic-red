#include <vulkan/vulkan.h>
#include <SDL3/SDL.h>
#include <SDL3/SDL_vulkan.h>

#define GLM_FORCE_RADIANS
#define GLM_FORCE_DEPTH_ZERO_TO_ONE
#include <glm/glm.hpp>
#include <glm/gtx/transform.hpp>

#define VMA_IMPLEMENTATION
#include <vk_mem_alloc.h>

#include <vulkan/vk_enum_string_helper.h> // Doesn't work on linux?
#include <Common/Debug.h>

#include <vector>
#include <set>
#include <stdexcept>
#include <cstdlib>

#include <Common/RootDir.h>
#include <Common/Platform.h>
#include <Common/Compiler/DisableWarnings.h>

#include <Control/Camera.h>
#include <Common/Log.h>
#include <DeletionQueue.h>
#include <Rendering/Mesh/Mesh.h>

#include <Rendering/Wrappers/Buffer.h>
#include <Rendering/Wrappers/Image.h>
#include <Rendering/Wrappers/ImageMemoryBarrier.h>
#include <Rendering/Wrappers/DynamicRendering.h>

#include <Rendering/Pipeline/MaterialFunctions.h>
#include <Rendering/Mesh/RenderObject.h>
#include <Rendering/Vertex/VertexDescriptors.h>
#include <Rendering/Vertex/Vertex.h>
#include <Common/Config.h>
#include <Common/Defaults.h>
#include <Managers/Scene.h>
#include <Light/LightManager.h>
#include <Rendering/Pipeline/GraphicsPipeline.h>
#include <Rendering/Mesh/MeshPushConstants.h>
#include <Rendering/Descriptor/Descriptor.h>

#include <IncludeHelpers/ImguiIncludes.h>

#if PLATFORM_WINDOWS
typedef void* HWND;
#elif PLATFORM_MACOS
extern void* GetNSWindowView(SDL_Window* wnd); // Written in the Objective C++ file SurfaceHelper.mm
typedef void* NSWindow;
#endif
PUSH_CLANG_WARNINGS
DISABLE_CLANG_WARNING("-Wunused-parameter")
DISABLE_CLANG_WARNING("-Wgnu-zero-variadic-macro-arguments")
// Diligent
#include <EngineFactoryVk.h>
#include <Common/interface/RefCntAutoPtr.hpp>
#include <Graphics/GraphicsTools/interface/MapHelper.hpp>
#include <Graphics/GraphicsTools/interface/GraphicsUtilities.h>
#include <FlagEnum.h>
POP_CLANG_WARNINGS


#include <Rendering/Shader/Shader.h>


// Frame data
int frameNumber = 0;
float deltaTime = 0.0f; // Time between current and last frame
uint64_t lastFrameTick = 0;
uint64_t currentFrameTick = 0;
bool interactableUI = false;

// Camera
glm::vec3 cameraPos = glm::vec3(0.0f, 0.0f, -2.0f);
glm::vec3 worldUp = glm::vec3(0.0f, 1.0f, 0.0f);
glm::vec3 cameraFront = glm::vec3(0.0f, 0.0f, -1.0f);
Camera camera(cameraPos, worldUp, cameraFront, -90.0f, 0.0f, 45.0f, true);
float cameraSpeed = 0.0f;
bool firstMouse = true;
float lastX = WINDOW_WIDTH / 2, lastY = WINDOW_HEIGHT / 2; // Initial mouse positions

class Engine {
public:
    void run() {
        initWindow();
        initVulkan();
        mainLoop();
        cleanup();
    }
// TODO: remove this lol
PUSH_CLANG_WARNINGS
DISABLE_CLANG_WARNING("-Wunused-private-field")
private:
    SDL_Window *window;

    // VulkanMemoryAllocator (VMA)
    VmaAllocator vmaAllocator;

    // Instance
    std::vector<const char*> extensionsVector;
    std::vector<const char*> layers;
    VkInstance instance;
    VkDebugUtilsMessengerEXT debugMessenger;

    // Surface and devices
    VkSurfaceKHR surface;
    VkPhysicalDevice physicalDevice;
    VkDevice device;

    // Cleanup
    DeletionQueue mainDeletionQueue; // Contains all deletable vulkan resources except pipelines/pipeline layouts
POP_CLANG_WARNINGS
#if PLATFORM_WINDOWS
    HWND hwnd;
    Diligent::Win32NativeWindow diligent_hwnd;
#elif PLATFORM_MACOS
    Diligent::MacOSNativeWindow diligent_nswindow;
#endif

    void initWindow() {
        // We initialize SDL and create a window with it.
        SDL_Init(SDL_INIT_VIDEO);

        window = SDL_CreateWindow("Magic Red", WINDOW_WIDTH, WINDOW_HEIGHT, SDL_WINDOW_VULKAN);
        SDL_SetRelativeMouseMode(SDL_TRUE);

#if defined(SDL_PLATFORM_WIN32)
        hwnd = (HWND)SDL_GetProperty(SDL_GetWindowProperties(window), SDL_PROP_WINDOW_WIN32_HWND_POINTER, NULL);
        diligent_hwnd = Diligent::Win32NativeWindow(hwnd);
#elif defined(SDL_PLATFORM_MACOS)
        NSWindow *nswindow = (NSWindow *)GetNSWindowView(window);
        diligent_nswindow = Diligent::MacOSNativeWindow(nswindow);
#endif
    }

Diligent::RefCntAutoPtr<Diligent::IRenderDevice>  m_pDevice;
Diligent::RefCntAutoPtr<Diligent::IDeviceContext> m_pImmediateContext;
Diligent::RefCntAutoPtr<Diligent::ISwapChain>     m_pSwapChain;
    void engineInit()
    {
        Diligent::SwapChainDesc swapchainDesc = Diligent::SwapChainDesc(
            WINDOW_WIDTH, WINDOW_HEIGHT, 
            Diligent::TEXTURE_FORMAT::TEX_FORMAT_BGRA8_UNORM, Diligent::TEXTURE_FORMAT::TEX_FORMAT_D32_FLOAT,
            3, //swapChainImageCount,
            1.f, // Default depth value
            0,   // Default stencil value
            true // Is primary
        );
        Diligent::EngineVkCreateInfo engineCI;
        auto* pFactoryVk = Diligent::GetEngineFactoryVk();
        pFactoryVk->CreateDeviceAndContextsVk(engineCI, &m_pDevice, &m_pImmediateContext);
        
        #if PLATFORM_WINDOWS
        pFactoryVk->CreateSwapChainVk(m_pDevice, m_pImmediateContext, swapchainDesc, diligent_hwnd, &m_pSwapChain);
        #elif PLATFORM_MACOS
        pFactoryVk->CreateSwapChainVk(m_pDevice, m_pImmediateContext, swapchainDesc, diligent_nswindow, &m_pSwapChain);
        #endif
        MRLOG("Op success!");
    }
    

Diligent::RefCntAutoPtr<Diligent::IPipelineState> m_pPSO;
Diligent::RefCntAutoPtr<Diligent::IShaderResourceBinding> m_pSRB;
Diligent::RefCntAutoPtr<Diligent::IBuffer> m_pVSCBConstants;
Diligent::RefCntAutoPtr<Diligent::IBuffer> m_pFSLightCBConstants;

    void init_scene_lights() {
        LightManager::GetInstance().add_point_light({glm::vec4(0.0f, 3.5f, -4.0f, 0.0f), glm::vec4(1.0f, 0.0f, 0.0f, 1.0f)});
        LightManager::GetInstance().add_point_light({glm::vec4(0.0f, 3.5f, -4.0f, 0.0f), glm::vec4(0.0f, 0.0f, 1.0f, 1.0f)});
    }

struct cb_contents
{
    glm::mat4 model;
    glm::mat4 view;
    glm::mat4 projection;
    // glm::vec4 pos;
    // glm::vec4 color;
};
    void buildResources()
    {
        // Per object onstant buffer creation, frequently updated by the CPU, used for our transformation matrices
         Diligent::BufferDesc constantBufferDesc;
         constantBufferDesc.Name = "VS constants CB";
         constantBufferDesc.Size = sizeof(cb_contents);
         constantBufferDesc.Usage = Diligent::USAGE_DYNAMIC;
         constantBufferDesc.BindFlags = Diligent::BIND_UNIFORM_BUFFER;
         constantBufferDesc.CPUAccessFlags = Diligent::CPU_ACCESS_WRITE;
         m_pDevice->CreateBuffer(constantBufferDesc, nullptr, &m_pVSCBConstants);
        //CreateUniformBuffer(m_pDevice, sizeof(glm::mat4), "VS constants CB", &m_pVSCBConstants);

        // Per scene constant buffer creation, used for lighting info
        Diligent::BufferDesc lightConstantBufferDesc;
        lightConstantBufferDesc.Name = "VS light constants CB";
        lightConstantBufferDesc.Size = LightManager::GetInstance().get_point_light_count() * sizeof(PointLight);
        lightConstantBufferDesc.Usage = Diligent::USAGE_DYNAMIC;
        lightConstantBufferDesc.BindFlags = Diligent::BIND_UNIFORM_BUFFER;
        lightConstantBufferDesc.CPUAccessFlags = Diligent::CPU_ACCESS_WRITE;
        m_pDevice->CreateBuffer(lightConstantBufferDesc, nullptr, &m_pFSLightCBConstants);
        //CreateUniformBuffer(m_pDevice, /*Scene::GetInstance().scenePointLights.size() * */sizeof(PointLight), "VS light constants CB", &m_pFSLightCBConstants);

        // Define how vertex attributes are fetched from the vertex buffer
        Diligent::LayoutElement LayoutElems[] =
        {
            // Attribute 0 - vertex position
            Diligent::LayoutElement{0, 0, 3, Diligent::VT_FLOAT32, Diligent::False},
            // Attribute 1 - vertex normal
            Diligent::LayoutElement{1, 0, 3, Diligent::VT_FLOAT32, Diligent::False},
            // Attribute 2 - vertex color
            Diligent::LayoutElement{2, 0, 4, Diligent::VT_FLOAT32, Diligent::False}
        };

        Diligent::GraphicsPipelineStateCreateInfo psoCreateInfo;
        psoCreateInfo.GraphicsPipeline.InputLayout.LayoutElements = LayoutElems;
        psoCreateInfo.GraphicsPipeline.InputLayout.NumElements = _countof(LayoutElems);

        //// Layout description
        //Diligent::ShaderResourceVariableDesc ShaderVars[] =
        //{
        //    {Diligent::SHADER_TYPE_VERTEX, "Constants",  Diligent::SHADER_RESOURCE_VARIABLE_TYPE_STATIC},
        //    {Diligent::SHADER_TYPE_VERTEX, "LightConstants", Diligent::SHADER_RESOURCE_VARIABLE_TYPE_STATIC},
        //};
        //psoCreateInfo.PSODesc.ResourceLayout.Variables = ShaderVars;
        //psoCreateInfo.PSODesc.ResourceLayout.NumVariables = _countof(ShaderVars);
        psoCreateInfo.PSODesc.ResourceLayout.DefaultVariableType = Diligent::SHADER_RESOURCE_VARIABLE_TYPE_STATIC;


        // Other
        psoCreateInfo.PSODesc.Name = "Simple Triangle PSO";
        psoCreateInfo.PSODesc.PipelineType = Diligent::PIPELINE_TYPE_GRAPHICS;
        psoCreateInfo.GraphicsPipeline.NumRenderTargets = 1;
        psoCreateInfo.GraphicsPipeline.RTVFormats[0] = m_pSwapChain->GetDesc().ColorBufferFormat;
        psoCreateInfo.GraphicsPipeline.DSVFormat = m_pSwapChain->GetDesc().DepthBufferFormat;
        psoCreateInfo.GraphicsPipeline.DepthStencilDesc.DepthEnable = Diligent::True;

        psoCreateInfo.GraphicsPipeline.RasterizerDesc.CullMode = Diligent::CULL_MODE_BACK;
        psoCreateInfo.GraphicsPipeline.RasterizerDesc.FrontCounterClockwise = Diligent::True;

        psoCreateInfo.GraphicsPipeline.PrimitiveTopology = Diligent::PRIMITIVE_TOPOLOGY_TRIANGLE_LIST;
        
        // bool load_shader_spirv_source_to_bytes(const std::string& shaderSpirvPath, std::vector<uint32_t> &byteBuffer, size_t &byteBufferSizeBytes)
        //std::vector<uint32_t> VS_source;
        //size_t VS_byteCount;
        //std::vector<uint32_t> PS_source;
        //size_t PS_byteCount;

        //load_shader_spirv_source_to_bytes(std::string(ROOT_DIR) + std::string("Shaders/hello_triangle.vert.spv"), VS_source, VS_byteCount);
        //load_shader_spirv_source_to_bytes(std::string(ROOT_DIR) + std::string("Shaders/hello_triangle.frag.spv"), PS_source, PS_byteCount);

        std::string VS_source;
        std::string PS_source;
        load_shader_source_to_string(std::string(ROOT_DIR) + std::string("Shaders/simple.vsh"), VS_source);
        load_shader_source_to_string(std::string(ROOT_DIR) + std::string("Shaders/simple.psh"), PS_source);

        Diligent::ShaderCreateInfo shaderCI;
        shaderCI.SourceLanguage = Diligent::SHADER_SOURCE_LANGUAGE_HLSL;
        shaderCI.Desc.UseCombinedTextureSamplers = true; // optional?
        Diligent::RefCntAutoPtr<Diligent::IShader> pVS;
        {
            shaderCI.Desc.ShaderType = Diligent::SHADER_TYPE_VERTEX;
            shaderCI.EntryPoint = "main";
            shaderCI.Desc.Name = "Triangle vertex shader";
            //shaderCI.ByteCode = VS_source.data();
            //shaderCI.ByteCodeSize = VS_byteCount;
            shaderCI.Source = VS_source.data();
            m_pDevice->CreateShader(shaderCI, &pVS);
        }
        Diligent::RefCntAutoPtr<Diligent::IShader> pPS;
        {
            shaderCI.Desc.ShaderType = Diligent::SHADER_TYPE_PIXEL;
            shaderCI.EntryPoint = "main";
            shaderCI.Desc.Name = "Triangle fragment shader";
            //shaderCI.ByteCode = PS_source.data();
            //shaderCI.ByteCodeSize = PS_byteCount;
            shaderCI.Source = PS_source.data();
            m_pDevice->CreateShader(shaderCI, &pPS);
        }

        // Finally, create the pipeline state
        psoCreateInfo.pVS = pVS;
        psoCreateInfo.pPS = pPS;
        m_pDevice->CreateGraphicsPipelineState(psoCreateInfo, &m_pPSO);

        // Bind static variables, in this case just the constant buffer fed to the vertex shader
        m_pPSO->GetStaticVariableByName(Diligent::SHADER_TYPE_VERTEX, "Constants")->Set(m_pVSCBConstants);
        m_pPSO->GetStaticVariableByName(Diligent::SHADER_TYPE_PIXEL, "LightConstants")->Set(m_pFSLightCBConstants);

        // Since our vertex shader uses shader resources (constant buffer), we need to create a shader resource binding object that will manage all required resource bindings:
        m_pPSO->CreateShaderResourceBinding(&m_pSRB, true);
    }

    Diligent::RefCntAutoPtr<Diligent::IBuffer> m_pMonkeyVertexBuffer;
    Diligent::RefCntAutoPtr<Diligent::IBuffer> m_pMonkeyIndexBuffer;
    uint32_t numIndicesTemp;
    void init_scene_meshes() {

        // // Sponza mesh
        // Mesh sponzaMesh;
        // load_mesh_from_gltf(sponzaMesh, ROOT_DIR "/Assets/Meshes/sponza-gltf/Sponza.gltf", false);
        // Scene::GetInstance().sceneMeshMap["sponza"] = upload_mesh(sponzaMesh, vmaAllocator, mainDeletionQueue);

        // RenderObject sponzaObject("defaultMaterial", "sponza");
        // glm::mat4 translate = glm::translate(glm::mat4{ 1.0f }, glm::vec3(0.0f, -5.0f, 0.0f));
        // glm::mat4 scale = glm::scale(glm::mat4{ 1.0 }, glm::vec3(0.05f, 0.05f, 0.05f));
        // sponzaObject.transformMatrix = translate * scale;

        // Scene::GetInstance().sceneRenderObjects.push_back(sponzaObject);


        // Suzanne mesh
        Mesh monkeyMesh;
        load_mesh_from_gltf(monkeyMesh, ROOT_DIR "/Assets/Meshes/suzanne.glb", true);

        // Load vertices into buffer
        {
            Diligent::BufferDesc monkeyVertexBuffDesc;
            monkeyVertexBuffDesc.Name = "Monkey vertex buffer";
            monkeyVertexBuffDesc.BindFlags = Diligent::BIND_VERTEX_BUFFER;
            monkeyVertexBuffDesc.Usage = Diligent::USAGE_IMMUTABLE;
            monkeyVertexBuffDesc.Size = monkeyMesh.vertices.size() * Vertex::sizeInBytes();
            Diligent::BufferData monkeyVBData;
            monkeyVBData.pData = monkeyMesh.vertices.data();
            monkeyVBData.DataSize = monkeyMesh.vertices.size() * Vertex::sizeInBytes();
            m_pDevice->CreateBuffer(monkeyVertexBuffDesc, &monkeyVBData, &m_pMonkeyVertexBuffer);
        }


        // Load indices into buffer
        {
            Diligent::BufferDesc monkeyIndexBuffDesc;
            monkeyIndexBuffDesc.Name = "Monkey index buffer";
            monkeyIndexBuffDesc.BindFlags = Diligent::BIND_INDEX_BUFFER;
            monkeyIndexBuffDesc.Usage = Diligent::USAGE_IMMUTABLE;
            monkeyIndexBuffDesc.Size = monkeyMesh.indices.size() * sizeof(uint32_t);
            Diligent::BufferData monkeyIBData;
            monkeyIBData.pData = monkeyMesh.indices.data();
            monkeyIBData.DataSize = monkeyMesh.indices.size() * sizeof(uint32_t);
            m_pDevice->CreateBuffer(monkeyIndexBuffDesc, &monkeyIBData, &m_pMonkeyIndexBuffer);
        }

        numIndicesTemp = static_cast<uint32_t>(monkeyMesh.indices.size());

        // Scene::GetInstance().sceneMeshMap["suzanne"] = upload_mesh(monkeyMesh, vmaAllocator, mainDeletionQueue);

        //RenderObject monkeyObject("defaultMaterial", "suzanne");
        //glm::mat4 monkeyTranslate = glm::translate(glm::mat4{ 1.0f }, glm::vec3(0.0f, 0.0f, 0.0f));
        //monkeyObject.transformMatrix = monkeyTranslate;

        //Scene::GetInstance().sceneRenderObjects.push_back(monkeyObject);

        // // Helmet mesh
        // Mesh helmetMesh;
        // load_mesh_from_gltf(helmetMesh, ROOT_DIR "/Assets/Meshes/DamagedHelmet.glb", true);
        // Scene::GetInstance().sceneMeshMap["helmet"] = upload_mesh(helmetMesh, vmaAllocator, mainDeletionQueue);

        // RenderObject helmetObject("defaultMaterial", "helmet");
        // glm::mat4 helmetTransform = glm::translate(glm::mat4{ 1.0f }, glm::vec3(0.0f, 3.0f, 0.0f));
        // helmetTransform = glm::rotate(helmetTransform, glm::radians(90.0f), glm::vec3(1.0, 0.0, 0.0));
        // helmetObject.transformMatrix = helmetTransform;

        // Scene::GetInstance().sceneRenderObjects.push_back(helmetObject);
    }

    void update_lights()
    {
        // Update point lights
        {
            Diligent::MapHelper<PointLight> LightBuffer(m_pImmediateContext, m_pFSLightCBConstants, Diligent::MAP_WRITE, Diligent::MAP_FLAG_DISCARD);

            LightManager::GetInstance().update_point_lights(frameNumber);
            for (uint32_t light_i = 0; light_i < LightManager::GetInstance().get_point_light_count(); light_i++)
            {
                const PointLight& pointLight = LightManager::GetInstance().get_point_light_at(light_i);
                LightBuffer[light_i] = pointLight;
            }
        }
    }


    void diligent_render()
    {
        // Calculate view projection matrix
        glm::mat4 view = camera.get_view_matrix();
        glm::mat4 projection = glm::perspective(glm::radians(70.f), (float)WINDOW_WIDTH / (float)WINDOW_HEIGHT, 0.1f, 200.0f);
        //projection[1][1] *= -1; // flips the model because Vulkan uses positive Y downwards
        //glm::mat4 viewProjectionMatrix = projection * view;

        glm::mat4 monkeyTranslate = glm::translate(glm::mat4{ 1.0f }, glm::vec3(0.0f, 0.0f, 0.0f));

        //glm::mat4 modelViewProjectionMatrix = viewProjectionMatrix * monkeyTranslate;

        // Update vertex shader constant buffer with matrix constants
        {
            Diligent::MapHelper<cb_contents> VSConstants(m_pImmediateContext, m_pVSCBConstants, Diligent::MAP_WRITE, Diligent::MAP_FLAG_DISCARD);
            cb_contents contents;
            contents.model = monkeyTranslate;
            contents.view = view;
            contents.projection = projection;
            *VSConstants = contents;
        }
        

        // Bind vertex and index buffers
        Uint64   offset = 0;
        Diligent::IBuffer* pBuffs[] = { m_pMonkeyVertexBuffer };
        m_pImmediateContext->SetVertexBuffers(0, 1, pBuffs, &offset, Diligent::RESOURCE_STATE_TRANSITION_MODE_TRANSITION, Diligent::SET_VERTEX_BUFFERS_FLAG_RESET);
        m_pImmediateContext->SetIndexBuffer(m_pMonkeyIndexBuffer, 0, Diligent::RESOURCE_STATE_TRANSITION_MODE_TRANSITION);
        


        // Clear the back buffer
        const float ClearColor[] = { 0.350f, 0.350f, 0.350f, 1.0f };
        // Let the engine perform required state transitions
        auto* pRTV = m_pSwapChain->GetCurrentBackBufferRTV();
        auto* pDSV = m_pSwapChain->GetDepthBufferDSV();
        m_pImmediateContext->SetRenderTargets(1, &pRTV, pDSV, Diligent::RESOURCE_STATE_TRANSITION_MODE_TRANSITION);
        m_pImmediateContext->ClearRenderTarget(pRTV, ClearColor, Diligent::RESOURCE_STATE_TRANSITION_MODE_TRANSITION);
        m_pImmediateContext->ClearDepthStencil(pDSV, Diligent::CLEAR_DEPTH_FLAG, 1.f, 0, Diligent::RESOURCE_STATE_TRANSITION_MODE_TRANSITION);

        // Set the pipeline state in the immediate context
        m_pImmediateContext->SetPipelineState(m_pPSO);

        // Very important: Commit shader resources
        m_pImmediateContext->CommitShaderResources(m_pSRB, Diligent::RESOURCE_STATE_TRANSITION_MODE_TRANSITION);

        //Diligent::DrawAttribs drawAttrs;
        //drawAttrs.NumVertices = 3; // We will render 3 vertices
        //m_pImmediateContext->Draw(drawAttrs);

        Diligent::DrawIndexedAttribs drawIndexAttrs;
        drawIndexAttrs.IndexType = Diligent::VT_UINT32;
        drawIndexAttrs.NumIndices = numIndicesTemp;
        // Verify the state of vertex and index buffers as well as consistence of 
        // render targets and correctness of draw command arguments
        drawIndexAttrs.Flags = Diligent::DRAW_FLAG_VERIFY_ALL;
        m_pImmediateContext->DrawIndexed(drawIndexAttrs);

        m_pSwapChain->Present(true ? 1 : 0);
        frameNumber++;
    }

    void initVulkan() {
        engineInit();
        init_scene_lights();
        buildResources();
        init_scene_meshes();
    }

    void mainLoop() {
        SDL_Event sdlEvent;
        bool bQuit = false;
        while (!bQuit) {
            // Handle events on queue
            while (SDL_PollEvent(&sdlEvent) != 0) {
                // ImGui_ImplSDL3_ProcessEvent(&sdlEvent);      
                if (sdlEvent.type == SDL_EVENT_QUIT) { // Built in Alt+F4 or hitting the 'x' button
                    SDL_SetRelativeMouseMode(SDL_FALSE); // Needed or else mouse freeze persists until clicking after closing app
                    bQuit = true;
                }
                // For single key presses
                if (sdlEvent.type == SDL_EVENT_KEY_DOWN) {
                    if (sdlEvent.key.keysym.sym == SDLK_TAB) {
                        interactableUI = !interactableUI;
                        if (!interactableUI) {
                            SDL_SetRelativeMouseMode(SDL_FALSE);
                            camera.freezeCamera();
                        } else {
                            SDL_SetRelativeMouseMode(SDL_TRUE);
                            camera.unfreezeCamera();
                        }
                    }
                }
                
                // Mouse motion
                if (sdlEvent.type == SDL_EVENT_MOUSE_MOTION) {
                    float xoffset = sdlEvent.motion.xrel;
                    float yoffset =  -sdlEvent.motion.yrel;
                    const float sensitivity = 0.1f;
                    xoffset *= sensitivity;
                    yoffset *= sensitivity;
                    camera.process_mouse_movement(xoffset, yoffset, true);
                }
            }
            update_lights();
            diligent_render();

            lastFrameTick = currentFrameTick;
            currentFrameTick = SDL_GetTicks();
            deltaTime = (currentFrameTick - lastFrameTick) / 1000.f;

            // For multi-press/holding down keys
            const Uint8* state = SDL_GetKeyboardState(nullptr);
            if (state[SDL_SCANCODE_LSHIFT]) {
                cameraSpeed = 20.0f;
            } else {
                cameraSpeed = 10.0f;
            }
            if (state[SDL_SCANCODE_ESCAPE]) {
                SDL_SetRelativeMouseMode(SDL_FALSE); // Needed or else mouse freeze persists until clicking after closing app
                bQuit = true;
            }
            if (state[SDL_SCANCODE_W]) {
                camera.process_keyboard_input(CameraMovementDirection::FORWARD, cameraSpeed * deltaTime);
            }
            if (state[SDL_SCANCODE_S]) {
                camera.process_keyboard_input(CameraMovementDirection::BACKWARD, cameraSpeed * deltaTime);
            }
            if (state[SDL_SCANCODE_A]) {
                camera.process_keyboard_input(CameraMovementDirection::LEFT, cameraSpeed * deltaTime);
            }
            if (state[SDL_SCANCODE_D]) {
                camera.process_keyboard_input(CameraMovementDirection::RIGHT, cameraSpeed * deltaTime);
            }
            if (state[SDL_SCANCODE_SPACE]) {
                camera.process_keyboard_input(CameraMovementDirection::UP, cameraSpeed * deltaTime);
            }
            if (state[SDL_SCANCODE_LCTRL]) {
                camera.process_keyboard_input(CameraMovementDirection::DOWN, cameraSpeed * deltaTime);
            }
        }
    }

    void cleanup() {
        // mainDeletionQueue.flush();

        SDL_DestroyWindow(window);
        SDL_Quit();
    }
};

int main() {
    Engine engine;

    try {
        engine.run();
    } catch (const std::exception& e) {
        std::cerr << "[EXCEPTION] " << e.what() << std::endl;
        return EXIT_FAILURE;
    }

    return EXIT_SUCCESS;
}
