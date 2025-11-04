#include "DX11Framework.h"
#include <string>
//#define RETURNFAIL(x) if(FAILED(x)) return x;
#define ThrowOnFail(x) if(FAILED(x)) throw new std::exception;

LRESULT CALLBACK WndProc(HWND hWnd, UINT message, WPARAM wParam, LPARAM lParam)
{
    PAINTSTRUCT ps;
    HDC hdc;

    switch (message)
    {
    case WM_PAINT:
        hdc = BeginPaint(hWnd, &ps);
        EndPaint(hWnd, &ps);
        break;

    case WM_DESTROY:
        PostQuitMessage(0);
        break;

    default:
        return DefWindowProc(hWnd, message, wParam, lParam);
    }

    return 0;
}

HRESULT DX11Framework::Initialise(HINSTANCE hInstance, int nShowCmd)
{
    HRESULT hr = S_OK;

    hr = CreateWindowHandle(hInstance, nShowCmd);
    if (FAILED(hr)) return E_FAIL;

    hr = CreateD3DDevice();
    if (FAILED(hr)) return E_FAIL;

    hr = CreateSwapChainAndFrameBuffer();
    if (FAILED(hr)) return E_FAIL;

    hr = InitShadersAndInputLayout();
    if (FAILED(hr)) return E_FAIL;

    hr = InitVertexIndexBuffers();
    if (FAILED(hr)) return E_FAIL;

    hr = InitPipelineVariables();
    if (FAILED(hr)) return E_FAIL;

    hr = InitRunTimeData();
    if (FAILED(hr)) return E_FAIL;

    return hr;
}

HRESULT DX11Framework::CreateWindowHandle(HINSTANCE hInstance, int nCmdShow)
{
    const wchar_t* windowName  = L"DX11Framework";

    WNDCLASSW wndClass;
    wndClass.style = 0;
    wndClass.lpfnWndProc = WndProc;
    wndClass.cbClsExtra = 0;
    wndClass.cbWndExtra = 0;
    wndClass.hInstance = 0;
    wndClass.hIcon = 0;
    wndClass.hCursor = 0;
    wndClass.hbrBackground = 0;
    wndClass.lpszMenuName = 0;
    wndClass.lpszClassName = windowName;

    RegisterClassW(&wndClass);

    _windowHandle = CreateWindowExW(0, windowName, windowName, WS_OVERLAPPEDWINDOW | WS_VISIBLE, CW_USEDEFAULT, CW_USEDEFAULT, 
        _WindowWidth, _WindowHeight, nullptr, nullptr, hInstance, nullptr);

    return S_OK;
}

HRESULT DX11Framework::CreateD3DDevice()
{
    HRESULT hr = S_OK;

    D3D_FEATURE_LEVEL featureLevels[] = {
        D3D_FEATURE_LEVEL_11_1, D3D_FEATURE_LEVEL_11_0,
        D3D_FEATURE_LEVEL_10_1, D3D_FEATURE_LEVEL_10_0,
    };

    ID3D11Device* baseDevice;
    ID3D11DeviceContext* baseDeviceContext;

    DWORD createDeviceFlags = 0;
#ifdef _DEBUG
    // If we're building for Debug, contain device warnings and errors
    // These are output to the visual studio console
    createDeviceFlags |= D3D11_CREATE_DEVICE_DEBUG;
#endif
    hr = D3D11CreateDevice(nullptr, D3D_DRIVER_TYPE_HARDWARE, nullptr, D3D11_CREATE_DEVICE_BGRA_SUPPORT | createDeviceFlags, featureLevels, ARRAYSIZE(featureLevels), D3D11_SDK_VERSION, &baseDevice, nullptr, &baseDeviceContext);
    if (FAILED(hr)) return hr;

    ///////////////////////////////////////////////////////////////////////////////////////////////

    hr = baseDevice->QueryInterface(__uuidof(ID3D11Device), reinterpret_cast<void**>(&_device));
    hr = baseDeviceContext->QueryInterface(__uuidof(ID3D11DeviceContext), reinterpret_cast<void**>(&_immediateContext));

    baseDevice->Release();
    baseDeviceContext->Release();

    ///////////////////////////////////////////////////////////////////////////////////////////////

    hr = _device->QueryInterface(__uuidof(IDXGIDevice), reinterpret_cast<void**>(&_dxgiDevice));
    if (FAILED(hr)) return hr;

    IDXGIAdapter* dxgiAdapter;
    hr = _dxgiDevice->GetAdapter(&dxgiAdapter);
    hr = dxgiAdapter->GetParent(__uuidof(IDXGIFactory2), reinterpret_cast<void**>(&_dxgiFactory));
    dxgiAdapter->Release();

    return S_OK;
}

HRESULT DX11Framework::CreateSwapChainAndFrameBuffer()
{
    HRESULT hr = S_OK;

    DXGI_SWAP_CHAIN_DESC1 swapChainDesc;
    swapChainDesc.Width = 0; // Defer to WindowWidth
    swapChainDesc.Height = 0; // Defer to WindowHeight
    swapChainDesc.Format = DXGI_FORMAT_R8G8B8A8_UNORM; //FLIP* modes don't support sRGB backbuffer
    swapChainDesc.Stereo = FALSE;
    swapChainDesc.SampleDesc.Count = 1;
    swapChainDesc.SampleDesc.Quality = 0;
    swapChainDesc.BufferUsage = DXGI_USAGE_RENDER_TARGET_OUTPUT;
    swapChainDesc.BufferCount = 2;
    swapChainDesc.Scaling = DXGI_SCALING_STRETCH;
    swapChainDesc.SwapEffect = DXGI_SWAP_EFFECT_FLIP_SEQUENTIAL;
    swapChainDesc.AlphaMode = DXGI_ALPHA_MODE_UNSPECIFIED;
    swapChainDesc.Flags = 0;

    hr = _dxgiFactory->CreateSwapChainForHwnd(_device, _windowHandle, &swapChainDesc, nullptr, nullptr, &_swapChain);
    if (FAILED(hr)) return hr;

    ///////////////////////////////////////////////////////////////////////////////////////////////

    ID3D11Texture2D* frameBuffer = nullptr;

    hr = _swapChain->GetBuffer(0, __uuidof(ID3D11Texture2D), reinterpret_cast<void**>(&frameBuffer));
    if (FAILED(hr)) return hr;

    D3D11_RENDER_TARGET_VIEW_DESC framebufferDesc = {};
    framebufferDesc.Format = DXGI_FORMAT_R8G8B8A8_UNORM_SRGB; //sRGB render target enables hardware gamma correction
    framebufferDesc.ViewDimension = D3D11_RTV_DIMENSION_TEXTURE2D;

    hr = _device->CreateRenderTargetView(frameBuffer, &framebufferDesc, &_frameBufferView);

    D3D11_TEXTURE2D_DESC depthBufferDesc = {};
    frameBuffer->GetDesc(&depthBufferDesc);

    depthBufferDesc.Format = DXGI_FORMAT_D24_UNORM_S8_UINT;
    depthBufferDesc.BindFlags = D3D11_BIND_DEPTH_STENCIL;

    _device->CreateTexture2D(&depthBufferDesc, nullptr, &_depthStencilBuffer);
    _device->CreateDepthStencilView(_depthStencilBuffer, nullptr, &_depthStencilView);


    frameBuffer->Release();


    return hr;
}

HRESULT DX11Framework::InitShadersAndInputLayout()
{
    HRESULT hr = S_OK;
    ID3DBlob* errorBlob;

    DWORD dwShaderFlags = D3DCOMPILE_ENABLE_STRICTNESS;
#ifdef _DEBUG
    // Set the D3DCOMPILE_DEBUG flag and D3DCOMPILE_SKIP_OPTIMIZATION
    // This means the shader runs slower, but debugging is easier
    // As optimizations will move/remove lines of code in the shader
    // https://learn.microsoft.com/en-us/windows/win32/direct3dhlsl/d3dcompile-constants
    dwShaderFlags |= D3DCOMPILE_DEBUG | D3DCOMPILE_SKIP_OPTIMIZATION;
#endif
    
    ID3DBlob* vsBlob;

    hr =  D3DCompileFromFile(L"SimpleShaders.hlsl", nullptr, D3D_COMPILE_STANDARD_FILE_INCLUDE, "VS_main", "vs_5_0", dwShaderFlags, 0, &vsBlob, &errorBlob);
    if (FAILED(hr))
    {
        MessageBoxA(_windowHandle, (char*)errorBlob->GetBufferPointer(), nullptr, ERROR);
        errorBlob->Release();
        return hr;
    }

    hr = _device->CreateVertexShader(vsBlob->GetBufferPointer(), vsBlob->GetBufferSize(), nullptr, &_vertexShader);

    if (FAILED(hr)) return hr;

    D3D11_INPUT_ELEMENT_DESC inputElementDesc[] =
    {
        { "POSITION", 0, DXGI_FORMAT_R32G32B32_FLOAT, 0, 0, D3D11_INPUT_PER_VERTEX_DATA,   0 },
        { "NORMAL", 0, DXGI_FORMAT_R32G32B32_FLOAT, 0, D3D11_APPEND_ALIGNED_ELEMENT, D3D11_INPUT_PER_VERTEX_DATA,   0 },
    };

    hr = _device->CreateInputLayout(inputElementDesc, ARRAYSIZE(inputElementDesc), vsBlob->GetBufferPointer(), vsBlob->GetBufferSize(), &_inputLayout);
    if (FAILED(hr)) return hr;

    ///////////////////////////////////////////////////////////////////////////////////////////////

    ID3DBlob* psBlob;

    hr = D3DCompileFromFile(L"SimpleShaders.hlsl", nullptr, D3D_COMPILE_STANDARD_FILE_INCLUDE, "PS_main", "ps_5_0", dwShaderFlags, 0, &psBlob, &errorBlob);
    if (FAILED(hr))
    {
        MessageBoxA(_windowHandle, (char*)errorBlob->GetBufferPointer(), nullptr, ERROR);
        errorBlob->Release();
        return hr;
    }

    hr = _device->CreatePixelShader(psBlob->GetBufferPointer(), psBlob->GetBufferSize(), nullptr, &_pixelShader);

    vsBlob->Release();
    psBlob->Release();

    return hr;
}

HRESULT DX11Framework::InitVertexIndexBuffers()
{
    HRESULT hr = S_OK;

    SimpleVertex VertexData[] = 
    {
        // Front Face
        { XMFLOAT3(-1.0f,  1.0f, -1.0f), XMFLOAT3(0.0f,  0.0f, -1.0f) },
        { XMFLOAT3(1.0f,  1.0f, -1.0f), XMFLOAT3(0.0f,  0.0f, -1.0f) },
        { XMFLOAT3(-1.0f, -1.0f, -1.0f), XMFLOAT3(0.0f,  0.0f, -1.0f) },
        { XMFLOAT3(1.0f, -1.0f, -1.0f), XMFLOAT3(0.0f,  0.0f, -1.0f) },

        // Right Face
        { XMFLOAT3(1.0f,  1.0f, -1.0f), XMFLOAT3(1.0f,  0.0f,  0.0f) },
        { XMFLOAT3(1.0f,  1.0f,  1.0f), XMFLOAT3(1.0f,  0.0f,  0.0f) },
        { XMFLOAT3(1.0f, -1.0f, -1.0f), XMFLOAT3(1.0f,  0.0f,  0.0f) },
        { XMFLOAT3(1.0f, -1.0f,  1.0f), XMFLOAT3(1.0f,  0.0f,  0.0f) },

        // Back Face
        { XMFLOAT3(-1.0f,  1.0f,  1.0f), XMFLOAT3(0.0f,  0.0f,  1.0f) },
        { XMFLOAT3(1.0f,  1.0f,  1.0f), XMFLOAT3(0.0f,  0.0f,  1.0f) },
        { XMFLOAT3(-1.0f, -1.0f,  1.0f), XMFLOAT3(0.0f,  0.0f,  1.0f) },
        { XMFLOAT3(1.0f, -1.0f,  1.0f), XMFLOAT3(0.0f,  0.0f,  1.0f) },

        // Left Face
        { XMFLOAT3(-1.0f,  1.0f,  1.0f), XMFLOAT3(-1.0f,  0.0f,  0.0f) },
        { XMFLOAT3(-1.0f,  1.0f, -1.0f), XMFLOAT3(-1.0f,  0.0f,  0.0f) },
        { XMFLOAT3(-1.0f, -1.0f,  1.0f), XMFLOAT3(-1.0f,  0.0f,  0.0f) },
        { XMFLOAT3(-1.0f, -1.0f, -1.0f), XMFLOAT3(-1.0f,  0.0f,  0.0f) },

        // Top Face
        { XMFLOAT3(-1.0f,  1.0f,  1.0f), XMFLOAT3(0.0f,  1.0f,  0.0f) },
        { XMFLOAT3(1.0f,  1.0f,  1.0f), XMFLOAT3(0.0f,  1.0f,  0.0f) },
        { XMFLOAT3(-1.0f,  1.0f, -1.0f), XMFLOAT3(0.0f,  1.0f,  0.0f) },
        { XMFLOAT3(1.0f,  1.0f, -1.0f), XMFLOAT3(0.0f,  1.0f,  0.0f) },

        // Bottom Face
        { XMFLOAT3(-1.0f, -1.0f,  1.0f), XMFLOAT3(0.0f, -1.0f,  0.0f) },
        { XMFLOAT3(1.0f, -1.0f,  1.0f), XMFLOAT3(0.0f, -1.0f,  0.0f) },
        { XMFLOAT3(-1.0f, -1.0f, -1.0f), XMFLOAT3(0.0f, -1.0f,  0.0f) },
        { XMFLOAT3(1.0f, -1.0f, -1.0f), XMFLOAT3(0.0f, -1.0f,  0.0f) },
    };

    SimpleVertex PyramidVertexData[] =
    {
        //Bottom Face
        { XMFLOAT3(-1.00f, -1.00f, 1.00f), XMFLOAT3(1.0f,  0.0f, 0.0f)},
        { XMFLOAT3(1.00f, -1.00f, 1.00f),XMFLOAT3(0.0f,  1.0f, 0.0f)},
        { XMFLOAT3(-1.00f, -1.00f, -1.00f), XMFLOAT3(0.0f,  0.0f, 1.0f)},
        { XMFLOAT3(1.00f, -1.00f, -1.00f),XMFLOAT3(1.0f,  1.0f, 1.0f)},

        //Front Face
        { XMFLOAT3(-1.00f, -1.00f, -1.00f), XMFLOAT3(0.0f,  0.0f, 1.0f)},
        { XMFLOAT3(0.0f, 1.0f, 0.50f), XMFLOAT3(1.0f,  0.0f, 0.0f)},
        { XMFLOAT3(1.00f, -1.00f, -1.00f),XMFLOAT3(1.0f,  1.0f, 1.0f)},

        //Right Face
        { XMFLOAT3(1.00f, -1.00f, -1.00f),XMFLOAT3(1.0f,  1.0f, 1.0f)},
        { XMFLOAT3(0.0f, 1.0f, 0.50f), XMFLOAT3(1.0f,  0.0f, 0.0f)},
        { XMFLOAT3(1.00f, -1.00f, 1.00f),XMFLOAT3(0.0f,  1.0f, 0.0f)},

        //Back Face
        { XMFLOAT3(1.00f, -1.00f, 1.00f),XMFLOAT3(0.0f,  1.0f, 0.0f)},
        { XMFLOAT3(0.0f, 1.0f, 0.50f), XMFLOAT3(1.0f,  0.0f, 0.0f)},
        { XMFLOAT3(-1.00f, -1.00f, 1.00f), XMFLOAT3(1.0f,  0.0f, 0.0f)},

        //Left Face
        { XMFLOAT3(-1.00f, -1.00f, 1.00f), XMFLOAT3(1.0f,  0.0f, 0.0f)},
        { XMFLOAT3(0.0f, 1.0f, 0.50f), XMFLOAT3(1.0f,  0.0f, 0.0f)},
        { XMFLOAT3(-1.00f, -1.00f, -1.00f), XMFLOAT3(0.0f,  0.0f, 1.0f)},

    };

    /*SimpleVertex linelist[] =
    {
        {XMFLOAT3(0,3,0), XMFLOAT4(1, 1, 1, 1) },
        {XMFLOAT3(0,4,0), XMFLOAT4(1, 1, 1, 1) },
    };*/

    D3D11_BUFFER_DESC vertexBufferDesc = {};
    vertexBufferDesc.ByteWidth = sizeof(VertexData);
    vertexBufferDesc.Usage = D3D11_USAGE_IMMUTABLE;
    vertexBufferDesc.BindFlags = D3D11_BIND_VERTEX_BUFFER;

    D3D11_SUBRESOURCE_DATA vertexData = { VertexData };

    hr = _device->CreateBuffer(&vertexBufferDesc, &vertexData, &_vertexBuffer);
    if (FAILED(hr)) return hr;

    D3D11_BUFFER_DESC pyramidvertexBufferDesc = {};
    pyramidvertexBufferDesc.ByteWidth = sizeof(PyramidVertexData);
    pyramidvertexBufferDesc.Usage = D3D11_USAGE_IMMUTABLE;
    pyramidvertexBufferDesc.BindFlags = D3D11_BIND_VERTEX_BUFFER;

    D3D11_SUBRESOURCE_DATA pyramidvertexData = { PyramidVertexData };

    hr = _device->CreateBuffer(&pyramidvertexBufferDesc, &pyramidvertexData, &_pyramidVertexBuffer);
    if (FAILED(hr)) return hr;

    /*D3D11_BUFFER_DESC linevertexBufferDesc = {};
    linevertexBufferDesc.ByteWidth = sizeof(linelist);
    linevertexBufferDesc.Usage = D3D11_USAGE_IMMUTABLE;
    linevertexBufferDesc.BindFlags = D3D11_BIND_VERTEX_BUFFER;

    D3D11_SUBRESOURCE_DATA linevertexData = { linelist };

    hr = _device->CreateBuffer(&linevertexBufferDesc, &linevertexData, &_lineVertexBuffer);
    if (FAILED(hr)) return hr;*/


    ///////////////////////////////////////////////////////////////////////////////////////////////

    WORD IndexData[] =
    {
        //Indices
        0, 1, 2,
        2, 1, 3,

        4, 5, 6,
        6, 5, 7,

        10, 9, 8,
        11,9, 10,

        12,13,14,
        14,13,15,

        16, 17, 18,
        18, 17, 19,

        22, 21, 20,
        23, 21, 22,
    };

    WORD PyramidIndexData[] =
    {
        3, 1, 0,
        3, 0, 2,

        4, 5, 6,

        7, 8, 9,

        10, 11, 12,

        13, 14, 15,

    };

    D3D11_BUFFER_DESC indexBufferDesc = {};
    indexBufferDesc.ByteWidth = sizeof(IndexData);
    indexBufferDesc.Usage = D3D11_USAGE_IMMUTABLE;
    indexBufferDesc.BindFlags = D3D11_BIND_INDEX_BUFFER;

    D3D11_SUBRESOURCE_DATA indexData = { IndexData };

    hr = _device->CreateBuffer(&indexBufferDesc, &indexData, &_indexBuffer);
    if (FAILED(hr)) return hr;


    D3D11_BUFFER_DESC pyramidindexBufferDesc = {};
    pyramidindexBufferDesc.ByteWidth = sizeof(PyramidIndexData);
    pyramidindexBufferDesc.Usage = D3D11_USAGE_IMMUTABLE;
    pyramidindexBufferDesc.BindFlags = D3D11_BIND_INDEX_BUFFER;

    D3D11_SUBRESOURCE_DATA pyramidData = { PyramidIndexData };

    hr = _device->CreateBuffer(&pyramidindexBufferDesc, &pyramidData, &_pyramidIndexBuffer);
    if (FAILED(hr)) return hr;

    return S_OK;
}

HRESULT DX11Framework::InitPipelineVariables()
{
    HRESULT hr = S_OK;

    //Input Assembler
    _immediateContext->IASetPrimitiveTopology(D3D11_PRIMITIVE_TOPOLOGY_TRIANGLELIST);
    _immediateContext->IASetInputLayout(_inputLayout);

    //Rasterizer
    D3D11_RASTERIZER_DESC rasterizerDesc = {};
    rasterizerDesc.FillMode = D3D11_FILL_SOLID;
    rasterizerDesc.CullMode = D3D11_CULL_BACK;

    hr = _device->CreateRasterizerState(&rasterizerDesc, &_fillState);
    if (FAILED(hr)) return hr;

    _immediateContext->RSSetState(_fillState); 

    D3D11_RASTERIZER_DESC wireframeDesc = {};
    wireframeDesc.FillMode = D3D11_FILL_WIREFRAME;
    wireframeDesc.CullMode = D3D11_CULL_NONE;

    hr = _device->CreateRasterizerState(&wireframeDesc, &_wireframeState);
    if (FAILED(hr)) return hr;

    _immediateContext->RSSetState(_fillState);

    //Viewport Values
    _viewport = { 0.0f, 0.0f, (float)_WindowWidth, (float)_WindowHeight, 0.0f, 1.0f };
    _immediateContext->RSSetViewports(1, &_viewport);

    //Constant Buffer
    D3D11_BUFFER_DESC constantBufferDesc = {};
    constantBufferDesc.ByteWidth = sizeof(ConstantBuffer);
    constantBufferDesc.Usage = D3D11_USAGE_DYNAMIC;
    constantBufferDesc.BindFlags = D3D11_BIND_CONSTANT_BUFFER;
    constantBufferDesc.CPUAccessFlags = D3D11_CPU_ACCESS_WRITE;

    hr = _device->CreateBuffer(&constantBufferDesc, nullptr, &_constantBuffer);
    if (FAILED(hr)) { return hr; }

    _immediateContext->VSSetConstantBuffers(0, 1, &_constantBuffer);
    _immediateContext->PSSetConstantBuffers(0, 1, &_constantBuffer);

    return S_OK;
}

HRESULT DX11Framework::InitRunTimeData()
{
    //Camera
    float aspect = _viewport.Width / _viewport.Height;

    XMFLOAT3 Eye = XMFLOAT3(0, 0, -10.0f);
    XMFLOAT3 At = XMFLOAT3(0, 0, 0);
    XMFLOAT3 Up = XMFLOAT3(0, 1, 0);

    _diffuseLight = XMFLOAT4(0.6f, 0.6f, 0.6f, 0.6f);
    _diffuseMaterial = XMFLOAT4(1.0f, 1.0f, 1.0f, 1.0f);
    _lightDir = XMFLOAT3(0, 0.5f, -0.5f);
    _ambientLight = XMFLOAT4(0.1f, 0.1f, 0.1f, 0.1f);
    _ambientMaterial = XMFLOAT4(0, 0, 0, 0);

    XMStoreFloat4x4(&_View, XMMatrixLookAtLH(XMLoadFloat3(&Eye), XMLoadFloat3(&At), XMLoadFloat3(&Up)));

    //Projection
    XMMATRIX perspective = XMMatrixPerspectiveFovLH(XMConvertToRadians(90), aspect, 0.01f, 100.0f);
    XMStoreFloat4x4(&_Projection, perspective);

    return S_OK;
}

DX11Framework::~DX11Framework()
{
    if(_immediateContext)_immediateContext->Release();
    if(_device)_device->Release();
    if(_dxgiDevice)_dxgiDevice->Release();
    if(_dxgiFactory)_dxgiFactory->Release();
    if(_frameBufferView)_frameBufferView->Release();
    if(_swapChain)_swapChain->Release();

    if (_wireframeState)_wireframeState->Release();
    if(_fillState)_fillState->Release();
    if(_vertexShader)_vertexShader->Release();
    if(_inputLayout)_inputLayout->Release();
    if(_pixelShader)_pixelShader->Release();
    if(_constantBuffer)_constantBuffer->Release();
    if(_vertexBuffer)_vertexBuffer->Release();
    if(_indexBuffer)_indexBuffer->Release();
    if(_depthStencilBuffer)_depthStencilBuffer->Release();
    if (_depthStencilView)_depthStencilView->Release();
    if (_pyramidIndexBuffer)_pyramidIndexBuffer->Release();
    if (_pyramidVertexBuffer)_pyramidVertexBuffer->Release();
    if (_lineVertexBuffer)_lineVertexBuffer->Release();
}


void DX11Framework::Update()
{
    //Static initializes this value only once    
    static ULONGLONG frameStart = GetTickCount64();

    ULONGLONG frameNow = GetTickCount64();
    float deltaTime = (frameNow - frameStart) / 1000.0f;
    frameStart = frameNow;

    static float simpleCount = 0.0f;
    simpleCount += deltaTime;
    _cbData.count = simpleCount;

    XMStoreFloat4x4(&_World, XMMatrixIdentity() * XMMatrixRotationX(simpleCount) * XMMatrixTranslation(0, sin(simpleCount), 0));

    XMStoreFloat4x4(&_World2, XMMatrixIdentity() * XMMatrixTranslation(4, sin(simpleCount), 2.5) * XMMatrixRotationX(simpleCount));

    XMMATRIX parent = XMMatrixMultiply(XMLoadFloat4x4(&_World2), XMMatrixTranslation(8, 0, 4));

    XMStoreFloat4x4(&_World3, parent  * XMMatrixScaling(0.5f, 0.5f, 0.5f) * XMMatrixRotationX(simpleCount) * XMMatrixTranslation(0, sin(simpleCount), 0));

    //XMStoreFloat4x4(&_World4, XMMatrixIdentity());
    

    if (GetAsyncKeyState(VK_F1) & 0x0001) 
    {
        _immediateContext->RSSetState(_wireframeState);
    }
}

void DX11Framework::Draw()
{    


    //Present unbinds render target, so rebind and clear at start of each frame
    float backgroundColor[4] = { 0.025f, 0.025f, 0.025f, 1.0f };  
    _immediateContext->IASetPrimitiveTopology(D3D11_PRIMITIVE_TOPOLOGY_TRIANGLELIST);
    _immediateContext->OMSetRenderTargets(1, &_frameBufferView, _depthStencilView);
    _immediateContext->ClearRenderTargetView(_frameBufferView, backgroundColor);
    _immediateContext->ClearDepthStencilView(_depthStencilView, D3D11_CLEAR_DEPTH | D3D11_CLEAR_STENCIL, 1.0f, 0.0f);
   
    //Store this frames data in constant buffer struct
    _cbData.World = XMMatrixTranspose(XMLoadFloat4x4(&_World));
    _cbData.View = XMMatrixTranspose(XMLoadFloat4x4(&_View));
    _cbData.Projection = XMMatrixTranspose(XMLoadFloat4x4(&_Projection));
    _cbData.DiffuseLight = _diffuseLight;
    _cbData.DiffuseMaterial = _diffuseMaterial;
    _cbData.LightDir = _lightDir;
    _cbData.AmbientLight = _ambientLight;
    _cbData.AmbientMaterial = _ambientMaterial;

    //Write constant buffer data onto GPU
    D3D11_MAPPED_SUBRESOURCE mappedSubresource;
    _immediateContext->Map(_constantBuffer, 0, D3D11_MAP_WRITE_DISCARD, 0, &mappedSubresource);
    memcpy(mappedSubresource.pData, &_cbData, sizeof(_cbData));
    _immediateContext->Unmap(_constantBuffer, 0);

    //Set object variables and draw
    UINT stride = {sizeof(SimpleVertex)};
    UINT offset =  0 ;
    _immediateContext->IASetVertexBuffers(0, 1, &_vertexBuffer, &stride, &offset);
    _immediateContext->IASetIndexBuffer(_indexBuffer, DXGI_FORMAT_R16_UINT, 0);

    _immediateContext->VSSetShader(_vertexShader, nullptr, 0);
    _immediateContext->PSSetShader(_pixelShader, nullptr, 0);

    _immediateContext->DrawIndexed(36, 0, 0);

    //Remap to update data Earth
    _immediateContext->Map(_constantBuffer, 0, D3D11_MAP_WRITE_DISCARD, 0, &mappedSubresource);

    //Load new world info
    _cbData.World = XMMatrixTranspose(XMLoadFloat4x4(&_World2));

    memcpy(mappedSubresource.pData, &_cbData, sizeof(_cbData));
    _immediateContext->Unmap(_constantBuffer, 0);


    _immediateContext->DrawIndexed(36, 0, 0);
     
    //Remap to update data Moon
    _immediateContext->Map(_constantBuffer, 0, D3D11_MAP_WRITE_DISCARD, 0, &mappedSubresource);

    //Load new world info
    _cbData.World = XMMatrixTranspose(XMLoadFloat4x4(&_World3));

    memcpy(mappedSubresource.pData, &_cbData, sizeof(_cbData));
    _immediateContext->Unmap(_constantBuffer, 0);

    _immediateContext->IASetVertexBuffers(0, 1, &_pyramidVertexBuffer, &stride, &offset);
    _immediateContext->IASetIndexBuffer(_pyramidIndexBuffer, DXGI_FORMAT_R16_UINT, 0);

    _immediateContext->DrawIndexed(18, 0, 0);

    //Remap to update data Line
    _immediateContext->Map(_constantBuffer, 0, D3D11_MAP_WRITE_DISCARD, 0, &mappedSubresource);

    //Load new world info
    _cbData.World = XMMatrixTranspose(XMLoadFloat4x4(&_World4));

    memcpy(mappedSubresource.pData, &_cbData, sizeof(_cbData));
    _immediateContext->Unmap(_constantBuffer, 0);

    _immediateContext->IASetPrimitiveTopology(D3D11_PRIMITIVE_TOPOLOGY_LINELIST);
    _immediateContext->IASetVertexBuffers(0, 1, &_lineVertexBuffer, &stride, &offset);
    _immediateContext->Draw(2, 0);

    //Present Backbuffer to screen
    _swapChain->Present(0, 0);
}