//*********************************************************
//
// Copyright (c) Microsoft. All rights reserved.
// This code is licensed under the MIT License (MIT).
// THIS CODE IS PROVIDED *AS IS* WITHOUT WARRANTY OF
// ANY KIND, EITHER EXPRESS OR IMPLIED, INCLUDING ANY
// IMPLIED WARRANTIES OF FITNESS FOR A PARTICULAR
// PURPOSE, MERCHANTABILITY, OR NON-INFRINGEMENT.
//
//*********************************************************

#include "stdafx.h"
#include "D3D12HelloTriangle.h"

D3D12HelloTriangle::D3D12HelloTriangle(UINT width, UINT height, std::wstring name) :
    DXSample(width, height, name),
    m_d3dCurrentFrameIndex(0),
    m_d3dViewport(0.0f, 0.0f, static_cast<float>(width), static_cast<float>(height)),
    m_d3dScissorRect(0, 0, static_cast<LONG>(width), static_cast<LONG>(height)),
    m_rtvDescriptorSize(0)
{
}

void D3D12HelloTriangle::OnInit()
{
    LoadPipeline();
    LoadAssets();
}

// Load the rendering pipeline dependencies.
void D3D12HelloTriangle::LoadPipeline()
{
    HRESULT hr = S_OK;

    UINT dxgiFactoryFlags = 0;

#if defined(_DEBUG)
    // Enable the debug layer (requires the Graphics Tools "optional feature").
    // NOTE: Enabling the debug layer after device creation will invalidate the active device.
    {
        ComPtr<ID3D12Debug> debugController;
        if (SUCCEEDED(D3D12GetDebugInterface(IID_PPV_ARGS(&debugController))))
        {
            debugController->EnableDebugLayer();
            // Enable additional debug layers.
            dxgiFactoryFlags |= DXGI_CREATE_FACTORY_DEBUG;
        }
    }
#endif

    ComPtr<IDXGIFactory4> factory;
    ThrowIfFailed(CreateDXGIFactory2(dxgiFactoryFlags, IID_PPV_ARGS(&factory)));

    if (m_useWarpDevice)
    {
        ComPtr<IDXGIAdapter> warpAdapter;
        ThrowIfFailed(factory->EnumWarpAdapter(IID_PPV_ARGS(&warpAdapter)));

        ThrowIfFailed(D3D12CreateDevice(warpAdapter.Get(), D3D_FEATURE_LEVEL_12_2, IID_PPV_ARGS(&m_d3dDevice) ));
    }
    else
    {
        ComPtr<IDXGIAdapter> hardwareAdapter = GetHardwareAdapter(factory.Get());

        ThrowIfFailed(D3D12CreateDevice(
            hardwareAdapter.Get(),
            D3D_FEATURE_LEVEL_12_2,
            IID_PPV_ARGS(&m_d3dDevice)
            ));
    }

    // Describe and create the command queue.
    D3D12_COMMAND_QUEUE_DESC queueDesc = {};
    queueDesc.Flags = D3D12_COMMAND_QUEUE_FLAG_NONE;
    queueDesc.Type = D3D12_COMMAND_LIST_TYPE_DIRECT;

    ThrowIfFailed(m_d3dDevice->CreateCommandQueue(&queueDesc, IID_PPV_ARGS(&m_d3dCommandQueue)));

    // Describe and create the swap chain.
    DXGI_SWAP_CHAIN_DESC1 swapChainDesc = {};
    swapChainDesc.BufferCount = FRAME_BUFFER_COUNT;
    swapChainDesc.Width = m_width;
    swapChainDesc.Height = m_height;
    swapChainDesc.Format = DXGI_FORMAT_R8G8B8A8_UNORM;
    swapChainDesc.BufferUsage = DXGI_USAGE_RENDER_TARGET_OUTPUT;
    swapChainDesc.SwapEffect = DXGI_SWAP_EFFECT_FLIP_DISCARD;
    swapChainDesc.SampleDesc.Count = 1;

    ThrowIfFailed(factory->CreateSwapChainForHwnd(
        m_d3dCommandQueue.Get(),        // Swap chain needs the queue so that it can force a flush on it.
        Win32Application::GetHwnd(),
        &swapChainDesc,
        nullptr,
        nullptr,
        (IDXGISwapChain1**)m_d3dSwapChain.GetAddressOf()
        ));

    // This sample does not support fullscreen transitions.
    ThrowIfFailed(factory->MakeWindowAssociation(Win32Application::GetHwnd(), DXGI_MWA_NO_ALT_ENTER));

    m_d3dCurrentFrameIndex = m_d3dSwapChain->GetCurrentBackBufferIndex();

    // Create descriptor heaps.
    {
        // Describe and create a render target view (RTV) descriptor heap.
        D3D12_DESCRIPTOR_HEAP_DESC rtvHeapDesc = {};
        rtvHeapDesc.NumDescriptors = FRAME_BUFFER_COUNT;
        rtvHeapDesc.Type = D3D12_DESCRIPTOR_HEAP_TYPE_RTV;
        rtvHeapDesc.Flags = D3D12_DESCRIPTOR_HEAP_FLAG_NONE;
        ThrowIfFailed(m_d3dDevice->CreateDescriptorHeap(&rtvHeapDesc, IID_PPV_ARGS(&m_rtvHeap)));

        m_rtvDescriptorSize = m_d3dDevice->GetDescriptorHandleIncrementSize(D3D12_DESCRIPTOR_HEAP_TYPE_RTV);
    }

    // Create frame resources.
    {
        CD3DX12_CPU_DESCRIPTOR_HANDLE rtvHandle(m_rtvHeap->GetCPUDescriptorHandleForHeapStart());

        // Create a RTV for each frame.
        for (UINT n = 0; n < FRAME_BUFFER_COUNT; n++)
        {
            ThrowIfFailed(m_d3dSwapChain->GetBuffer(n, IID_PPV_ARGS(&m_d3dRenderTarget[n])));
            m_d3dDevice->CreateRenderTargetView(m_d3dRenderTarget[n].Get(), nullptr, rtvHandle);
            rtvHandle.Offset(1, m_rtvDescriptorSize);
        }
    }

    ThrowIfFailed(m_d3dDevice->CreateCommandAllocator(D3D12_COMMAND_LIST_TYPE_DIRECT, IID_PPV_ARGS(&m_d3dCommandAllocator)));
}


std::wstring StringToWString(const std::string& str)
{
	int len = MultiByteToWideChar(CP_UTF8, 0, str.c_str(), -1, {}, 0);
	std::wstring wstr(len, 0);
	MultiByteToWideChar(CP_UTF8, 0, str.c_str(), -1, &wstr[0], len);
	return wstr;
}

HRESULT DXCompileShaderFromFile(const std::string& szFileName, const std::string& szEntryPoint, const std::string& szShaderModel, ID3DBlob** ppBlobOut)
{
	HRESULT hr = S_OK;
	UINT compileFlags = 0;
#ifdef _DEBUG
	compileFlags = D3DCOMPILE_DEBUG | D3DCOMPILE_SKIP_OPTIMIZATION;
#endif
	auto wFileName = StringToWString(szFileName);
	ID3DBlob* pErrorBlob{};
	hr = D3DCompileFromFile(wFileName.c_str(), {}, {}, szEntryPoint.c_str(), szShaderModel.c_str(), compileFlags, 0, ppBlobOut, &pErrorBlob);
	if (FAILED(hr))
	{
		if (pErrorBlob)
		{
			OutputDebugStringA(reinterpret_cast<const char*>(pErrorBlob->GetBufferPointer()));
			SAFE_RELEASE(pErrorBlob);
		}
		return hr;
	}
	SAFE_RELEASE(pErrorBlob);

	return S_OK;
}


// Load the sample assets.
void D3D12HelloTriangle::LoadAssets()
{
    HRESULT hr = S_OK;
    // Create an empty root signature.
	{
		D3D12_ROOT_SIGNATURE_DESC rsDesc{};
		rsDesc.Flags = D3D12_ROOT_SIGNATURE_FLAG_ALLOW_INPUT_ASSEMBLER_INPUT_LAYOUT;
		ComPtr<ID3DBlob> signature{}, error{};
		hr = D3D12SerializeRootSignature(&rsDesc, D3D_ROOT_SIGNATURE_VERSION_1, &signature, &error);
		if (FAILED(hr))
			return;
		hr = m_d3dDevice->CreateRootSignature(0, signature->GetBufferPointer(), signature->GetBufferSize(), IID_PPV_ARGS(&m_d3dRootSignature));
		if (FAILED(hr))
			return;
	}

    // Create the pipeline state, which includes compiling and loading shaders.
    {
        ComPtr<ID3DBlob> shaderVtx{}, shaderPxl{};

        hr = DXCompileShaderFromFile("E:/_doc/lessson_unreal/DirectX12-Graphics-Samples/Samples/Desktop/D3D12HelloWorld/src/HelloTriangle/shaders.hlsl", "main_vs", "vs_5_0", &shaderVtx);
        hr = DXCompileShaderFromFile("E:/_doc/lessson_unreal/DirectX12-Graphics-Samples/Samples/Desktop/D3D12HelloWorld/src/HelloTriangle/shaders.hlsl", "main_ps", "ps_5_0", &shaderPxl);

        // Define the vertex input layout.
        D3D12_INPUT_ELEMENT_DESC inputElementDescs[] =
        {
            { "POSITION", 0, DXGI_FORMAT_R32G32B32_FLOAT, 0, 0, D3D12_INPUT_CLASSIFICATION_PER_VERTEX_DATA, 0 },
            { "COLOR", 0, DXGI_FORMAT_R8G8B8A8_UNORM, 0, 0+sizeof(XMFLOAT3), D3D12_INPUT_CLASSIFICATION_PER_VERTEX_DATA, 0 }
        };


        // Describe and create the graphics pipeline state object (PSO).
		D3D12_RASTERIZER_DESC rasterDesc = {};
		    rasterDesc.FillMode = D3D12_FILL_MODE_SOLID;
		    rasterDesc.CullMode = D3D12_CULL_MODE_BACK;
		    rasterDesc.FrontCounterClockwise = FALSE;
		    rasterDesc.DepthBias = D3D12_DEFAULT_DEPTH_BIAS;
		    rasterDesc.DepthBiasClamp = D3D12_DEFAULT_DEPTH_BIAS_CLAMP;
		    rasterDesc.SlopeScaledDepthBias = D3D12_DEFAULT_SLOPE_SCALED_DEPTH_BIAS;
		    rasterDesc.DepthClipEnable = TRUE;
		    rasterDesc.MultisampleEnable = FALSE;
		    rasterDesc.AntialiasedLineEnable = FALSE;
		    rasterDesc.ForcedSampleCount = 0;
		    rasterDesc.ConservativeRaster = D3D12_CONSERVATIVE_RASTERIZATION_MODE_OFF;

		D3D12_BLEND_DESC blendDesc = {};
		    blendDesc.AlphaToCoverageEnable = FALSE;
		    blendDesc.IndependentBlendEnable = FALSE;
		    const D3D12_RENDER_TARGET_BLEND_DESC defaultRenderTargetBlendDesc =
            {
			    FALSE, FALSE,
			    D3D12_BLEND_ONE, D3D12_BLEND_ZERO, D3D12_BLEND_OP_ADD,
			    D3D12_BLEND_ONE, D3D12_BLEND_ZERO, D3D12_BLEND_OP_ADD,
			    D3D12_LOGIC_OP_NOOP,
			    D3D12_COLOR_WRITE_ENABLE_ALL
		    };
		    for (int i = 0; i < D3D12_SIMULTANEOUS_RENDER_TARGET_COUNT; ++i)
		    {
			    blendDesc.RenderTarget[i] = defaultRenderTargetBlendDesc;
		    }
		

		D3D12_GRAPHICS_PIPELINE_STATE_DESC psoDesc = {};
		    psoDesc.InputLayout = { inputElementDescs, _countof(inputElementDescs) };
		    psoDesc.pRootSignature = m_d3dRootSignature.Get();
		    psoDesc.VS = { shaderVtx.Get()->GetBufferPointer(), shaderVtx.Get()->GetBufferSize() };
		    psoDesc.PS = { shaderPxl.Get()->GetBufferPointer(), shaderPxl.Get()->GetBufferSize() };
		    psoDesc.BlendState = blendDesc;
		    psoDesc.RasterizerState = rasterDesc;
		    psoDesc.DepthStencilState.DepthEnable = FALSE;
		    psoDesc.DepthStencilState.StencilEnable = FALSE;
		    psoDesc.SampleMask = UINT_MAX;
		    psoDesc.PrimitiveTopologyType = D3D12_PRIMITIVE_TOPOLOGY_TYPE_TRIANGLE;
		    psoDesc.NumRenderTargets = 1;
		    psoDesc.RTVFormats[0] = DXGI_FORMAT_R8G8B8A8_UNORM;
		    psoDesc.SampleDesc.Count = 1;

        ThrowIfFailed(m_d3dDevice->CreateGraphicsPipelineState(&psoDesc, IID_PPV_ARGS(&m_pipelineState)));
    }

    // Create the command list.
    ThrowIfFailed(m_d3dDevice->CreateCommandList(0, D3D12_COMMAND_LIST_TYPE_DIRECT, m_d3dCommandAllocator.Get(), m_pipelineState.Get(), IID_PPV_ARGS(&m_d3dCommandList)));

    // Command lists are created in the recording state, but there is nothing
    // to record yet. The main loop expects it to be closed, so close it now.
    ThrowIfFailed(m_d3dCommandList->Close());

    // Create the vertex buffer.
    {
        m_bufVtxCount = 4;
        // Note: using upload heaps to transfer static data like vert buffers is not 
        // recommended. Every time the GPU needs it, the upload heap will be marshalled 
        // over. Please read up on Default Heap usage. An upload heap is used here for 
        // code simplicity and because there are very few verts to actually transfer.
		D3D12_HEAP_PROPERTIES heapProps = {};
		    heapProps.Type = D3D12_HEAP_TYPE_UPLOAD;
		    heapProps.CPUPageProperty = D3D12_CPU_PAGE_PROPERTY_UNKNOWN;
		    heapProps.MemoryPoolPreference = D3D12_MEMORY_POOL_UNKNOWN;
		    heapProps.CreationNodeMask = 1;
		    heapProps.VisibleNodeMask = 1;

		D3D12_RESOURCE_DESC resourceDesc = {};
		    resourceDesc.Dimension = D3D12_RESOURCE_DIMENSION_BUFFER;
		    resourceDesc.Alignment = 0;
            resourceDesc.Width = sizeof(Vertex) * m_bufVtxCount;
		    resourceDesc.Height = 1;
		    resourceDesc.DepthOrArraySize = 1;
		    resourceDesc.MipLevels = 1;
		    resourceDesc.Format = DXGI_FORMAT_UNKNOWN;
		    resourceDesc.SampleDesc.Count = 1;
		    resourceDesc.SampleDesc.Quality = 0;
		    resourceDesc.Layout = D3D12_TEXTURE_LAYOUT_ROW_MAJOR;
		    resourceDesc.Flags = D3D12_RESOURCE_FLAG_NONE;

		HRESULT hr = m_d3dDevice->CreateCommittedResource(
			&heapProps,
			D3D12_HEAP_FLAG_NONE,
			&resourceDesc,
			D3D12_RESOURCE_STATE_GENERIC_READ,
			nullptr,
			IID_PPV_ARGS(&m_rscVtx));
        ThrowIfFailed(hr);

        /*ThrowIfFailed(m_d3dDevice->CreateCommittedResource(
            &CD3DX12_HEAP_PROPERTIES(D3D12_HEAP_TYPE_UPLOAD),
            D3D12_HEAP_FLAG_NONE,
            &CD3DX12_RESOURCE_DESC::Buffer(vertexBufferSize),
            D3D12_RESOURCE_STATE_GENERIC_READ,
            nullptr,
            IID_PPV_ARGS(&m_rscVtx)));*/

        // Copy the triangle data to the vertex buffer.
        UINT8* pVertexDataBegin;
        D3D12_RANGE readRange{ 0, 0 };        // We do not intend to read from this resource on the CPU.
        ThrowIfFailed(m_rscVtx->Map(0, &readRange, reinterpret_cast<void**>(&pVertexDataBegin)));

		Vertex* pCur = reinterpret_cast<Vertex*>(pVertexDataBegin);
		*pCur++ = { { -0.25f,  0.25f * m_aspectRatio, 0.0f }, {   0,   0, 255, 255 } };
		*pCur++ = { {  0.25f,  0.25f * m_aspectRatio, 0.0f }, { 255,   0,   0, 255 } };
		*pCur++ = { {  0.25f, -0.25f * m_aspectRatio, 0.0f }, {   0, 255,   0, 255 } };
		*pCur++ = { { -0.25f, -0.25f * m_aspectRatio, 0.0f }, {   0, 128, 128, 255 } };
        // 이거 Unmap 안해도 돼
        //m_rscVtx->Unmap(0, nullptr);

        // Initialize the vertex buffer view.
        m_vtxView.BufferLocation = m_rscVtx->GetGPUVirtualAddress();
        m_vtxView.StrideInBytes = sizeof(Vertex);
        m_vtxView.SizeInBytes = m_bufVtxCount * m_vtxView.StrideInBytes;
    }

    // Create synchronization objects and wait until assets have been uploaded to the GPU.
    {
        ThrowIfFailed(m_d3dDevice->CreateFence(0, D3D12_FENCE_FLAG_NONE, IID_PPV_ARGS(&m_fence)));
        m_fenceValue = 1;

        // Create an event handle to use for frame synchronization.
        m_fenceEvent = CreateEvent(nullptr, FALSE, FALSE, nullptr);
        if (m_fenceEvent == nullptr)
        {
            ThrowIfFailed(HRESULT_FROM_WIN32(GetLastError()));
        }

        // Wait for the command list to execute; we are reusing the same command 
        // list in our main loop but for now, we just want to wait for setup to 
        // complete before continuing.
        WaitForPreviousFrame();
    }
}

// Update frame-based values.
void D3D12HelloTriangle::OnUpdate()
{
}

// Render the scene.
void D3D12HelloTriangle::OnRender()
{
	// Command list allocators can only be reset when the associated 
   // command lists have finished execution on the GPU; apps should use 
   // fences to determine GPU execution progress.
	ThrowIfFailed(m_d3dCommandAllocator->Reset());

	// However, when ExecuteCommandList() is called on a particular command 
	// list, that command list can then be reset at any time and must be before 
	// re-recording.
	ThrowIfFailed(m_d3dCommandList->Reset(m_d3dCommandAllocator.Get(), m_pipelineState.Get()));

	// Set necessary state.
	m_d3dCommandList->SetGraphicsRootSignature(m_d3dRootSignature.Get());
	m_d3dCommandList->RSSetViewports(1, &m_d3dViewport);
	m_d3dCommandList->RSSetScissorRects(1, &m_d3dScissorRect);

	// Indicate that the back buffer will be used as a render target.
	m_d3dCommandList->ResourceBarrier(1, &CD3DX12_RESOURCE_BARRIER::Transition(m_d3dRenderTarget[m_d3dCurrentFrameIndex].Get(), D3D12_RESOURCE_STATE_PRESENT, D3D12_RESOURCE_STATE_RENDER_TARGET));

	CD3DX12_CPU_DESCRIPTOR_HANDLE rtvHandle(m_rtvHeap->GetCPUDescriptorHandleForHeapStart(), m_d3dCurrentFrameIndex, m_rtvDescriptorSize);
	m_d3dCommandList->OMSetRenderTargets(1, &rtvHandle, FALSE, nullptr);

	// Record commands.
	const float clearColor[] = { 0.0f, 0.2f, 0.4f, 1.0f };
	m_d3dCommandList->ClearRenderTargetView(rtvHandle, clearColor, 0, nullptr);
	m_d3dCommandList->IASetPrimitiveTopology(D3D_PRIMITIVE_TOPOLOGY_TRIANGLEFAN);
	m_d3dCommandList->IASetVertexBuffers(0, 1, &m_vtxView);
	m_d3dCommandList->DrawInstanced(m_bufVtxCount, 1, 0, 0);

	// Indicate that the back buffer will now be used to present.
	m_d3dCommandList->ResourceBarrier(1, &CD3DX12_RESOURCE_BARRIER::Transition(m_d3dRenderTarget[m_d3dCurrentFrameIndex].Get(), D3D12_RESOURCE_STATE_RENDER_TARGET, D3D12_RESOURCE_STATE_PRESENT));

	ThrowIfFailed(m_d3dCommandList->Close());



    // Execute the command list.
    ID3D12CommandList* ppCommandLists[] = { m_d3dCommandList.Get() };
    m_d3dCommandQueue->ExecuteCommandLists(_countof(ppCommandLists), ppCommandLists);

    // Present the frame.
    ThrowIfFailed(m_d3dSwapChain->Present(1, 0));

    WaitForPreviousFrame();
}

void D3D12HelloTriangle::OnDestroy()
{
    // Ensure that the GPU is no longer referencing resources that are about to be
    // cleaned up by the destructor.
    WaitForPreviousFrame();

    CloseHandle(m_fenceEvent);
}

void D3D12HelloTriangle::PopulateCommandList()
{
    // Command list allocators can only be reset when the associated 
    // command lists have finished execution on the GPU; apps should use 
    // fences to determine GPU execution progress.
    ThrowIfFailed(m_d3dCommandAllocator->Reset());

    // However, when ExecuteCommandList() is called on a particular command 
    // list, that command list can then be reset at any time and must be before 
    // re-recording.
    ThrowIfFailed(m_d3dCommandList->Reset(m_d3dCommandAllocator.Get(), m_pipelineState.Get()));

    // Set necessary state.
    m_d3dCommandList->SetGraphicsRootSignature(m_d3dRootSignature.Get());
    m_d3dCommandList->RSSetViewports(1, &m_d3dViewport);
    m_d3dCommandList->RSSetScissorRects(1, &m_d3dScissorRect);

    // Indicate that the back buffer will be used as a render target.
    m_d3dCommandList->ResourceBarrier(1, &CD3DX12_RESOURCE_BARRIER::Transition(m_d3dRenderTarget[m_d3dCurrentFrameIndex].Get(), D3D12_RESOURCE_STATE_PRESENT, D3D12_RESOURCE_STATE_RENDER_TARGET));

    CD3DX12_CPU_DESCRIPTOR_HANDLE rtvHandle(m_rtvHeap->GetCPUDescriptorHandleForHeapStart(), m_d3dCurrentFrameIndex, m_rtvDescriptorSize);
    m_d3dCommandList->OMSetRenderTargets(1, &rtvHandle, FALSE, nullptr);

    // Record commands.
    const float clearColor[] = { 0.0f, 0.2f, 0.4f, 1.0f };
    m_d3dCommandList->ClearRenderTargetView(rtvHandle, clearColor, 0, nullptr);
    m_d3dCommandList->IASetPrimitiveTopology(D3D_PRIMITIVE_TOPOLOGY_TRIANGLELIST);
    m_d3dCommandList->IASetVertexBuffers(0, 1, &m_vtxView);
    m_d3dCommandList->DrawInstanced(3, 1, 0, 0);

    // Indicate that the back buffer will now be used to present.
    m_d3dCommandList->ResourceBarrier(1, &CD3DX12_RESOURCE_BARRIER::Transition(m_d3dRenderTarget[m_d3dCurrentFrameIndex].Get(), D3D12_RESOURCE_STATE_RENDER_TARGET, D3D12_RESOURCE_STATE_PRESENT));

    ThrowIfFailed(m_d3dCommandList->Close());
}

void D3D12HelloTriangle::WaitForPreviousFrame()
{
    // WAITING FOR THE FRAME TO COMPLETE BEFORE CONTINUING IS NOT BEST PRACTICE.
    // This is code implemented as such for simplicity. The D3D12HelloFrameBuffering
    // sample illustrates how to use fences for efficient resource usage and to
    // maximize GPU utilization.

    // Signal and increment the fence value.
    const UINT64 fence = m_fenceValue;
    ThrowIfFailed(m_d3dCommandQueue->Signal(m_fence.Get(), fence));
    m_fenceValue++;

    // Wait until the previous frame is finished.
    if (m_fence->GetCompletedValue() < fence)
    {
        ThrowIfFailed(m_fence->SetEventOnCompletion(fence, m_fenceEvent));
        WaitForSingleObject(m_fenceEvent, INFINITE);
    }

    m_d3dCurrentFrameIndex = m_d3dSwapChain->GetCurrentBackBufferIndex();
}
