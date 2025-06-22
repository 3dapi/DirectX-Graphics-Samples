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

#pragma once

#include "DXSample.h"

using namespace DirectX;

// Note that while ComPtr is used to manage the lifetime of resources on the CPU,
// it has no understanding of the lifetime of resources on the GPU. Apps must account
// for the GPU lifetime of resources to avoid destroying objects that may still be
// referenced by the GPU.
// An example of this can be found in the class method: OnDestroy().
using Microsoft::WRL::ComPtr;

class D3D12HelloTriangle : public DXSample
{
public:
    D3D12HelloTriangle(UINT width, UINT height, std::wstring name);

    virtual void OnInit();
    virtual void OnUpdate();
    virtual void OnRender();
    virtual void OnDestroy();

private:
    static const UINT FRAME_BUFFER_COUNT = 2;

    struct Vertex
    {
        XMFLOAT3 position;
        uint8_t color[4];
    };

    // Pipeline objects.
    CD3DX12_VIEWPORT                        m_d3dViewport;
    CD3DX12_RECT                            m_d3dScissorRect;
    ComPtr<IDXGISwapChain3>                 m_d3dSwapChain;
    ComPtr<ID3D12Device>                    m_d3dDevice;
    ComPtr<ID3D12Resource>                  m_d3dRenderTarget[FRAME_BUFFER_COUNT];
    ComPtr<ID3D12CommandAllocator>          m_d3dCommandAllocator;
    ComPtr<ID3D12CommandQueue>              m_d3dCommandQueue;
    ComPtr<ID3D12RootSignature>             m_d3dRootSignature;
    ComPtr<ID3D12DescriptorHeap>            m_rtvHeap;
    ComPtr<ID3D12PipelineState>             m_pipelineState;
    ComPtr<ID3D12GraphicsCommandList>       m_d3dCommandList;
    UINT                                    m_rtvDescriptorSize;

    int                                     m_bufVtxCount{};
    // App resources.
    ComPtr<ID3D12Resource>                  m_rscVtx;
    D3D12_VERTEX_BUFFER_VIEW                m_vtxView;

    // Synchronization objects.
    UINT                                    m_d3dCurrentFrameIndex;
    HANDLE                                  m_fenceEvent;
    ComPtr<ID3D12Fence>                     m_fence;
    UINT64 m_fenceValue;

    void LoadPipeline();
    void LoadAssets();
    void PopulateCommandList();
    void WaitForPreviousFrame();
};
