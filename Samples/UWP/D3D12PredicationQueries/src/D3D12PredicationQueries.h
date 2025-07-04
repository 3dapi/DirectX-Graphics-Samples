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

// Illustrate how queries are used in conjunction with predication.
class D3D12PredicationQueries : public DXSample
{
public:
    D3D12PredicationQueries(UINT width, UINT height, std::wstring name);

    virtual void OnInit();
    virtual void OnUpdate();
    virtual void OnRender();
    virtual void OnDestroy();

private:
    static const UINT m_RenderTargetCount = 2;
    static const UINT CbvCountPerFrame = 2;

    // Vertex definition.
    struct Vertex
    {
        XMFLOAT3 position;
        XMFLOAT4 color;
    };

    // Constant buffer definition.
    struct SceneConstantBuffer
    {
        XMFLOAT4 offset;

        // Constant buffers are 256-byte aligned. Add padding in the struct to allow multiple buffers
        // to be array-indexed.
        FLOAT padding[60];
    };

    // Each geometry gets its own constant buffer.
    SceneConstantBuffer m_constantBufferData[CbvCountPerFrame];
    UINT8* m_pCbvDataBegin;

    // Pipeline objects.
    CD3DX12_VIEWPORT m_viewport;
    CD3DX12_RECT m_scissorRect;
    ComPtr<IDXGISwapChain3> m_d3dSwapChain;
    ComPtr<ID3D12Device> m_d3dDevice;
    ComPtr<ID3D12Resource> m_d3dRenderTarget[m_RenderTargetCount];
    ComPtr<ID3D12CommandAllocator> m_d3dCommandAllocators[m_RenderTargetCount];
    ComPtr<ID3D12CommandQueue> m_d3dCommandQueue;
    ComPtr<ID3D12RootSignature> m_d3dRootSignature;
    ComPtr<ID3D12DescriptorHeap> m_d3dDecsHeap;
    ComPtr<ID3D12DescriptorHeap> m_cbvHeap;
    ComPtr<ID3D12DescriptorHeap> m_dsvHeap;
    ComPtr<ID3D12QueryHeap> m_queryHeap;
    UINT m_d3dDescriptorSize;
    UINT m_cbvSrvDescriptorSize;
    UINT m_frameIndex;

    // Synchronization objects.
    ComPtr<ID3D12Fence> m_fence;
    UINT64 m_fenceValues[m_RenderTargetCount];
    HANDLE m_fenceEvent;

    // Asset objects.
    ComPtr<ID3D12PipelineState> m_d3dPipelineState;
    ComPtr<ID3D12PipelineState> m_queryState;
    ComPtr<ID3D12GraphicsCommandList> m_d3dCommandList;
    ComPtr<ID3D12Resource> m_vertexBuffer;
    ComPtr<ID3D12Resource> m_constantBuffer;
    ComPtr<ID3D12Resource> m_d3dDepthStencil;
    ComPtr<ID3D12Resource> m_queryResult;
    D3D12_VERTEX_BUFFER_VIEW m_vertexBufferView;

    void LoadPipeline();
    void LoadAssets();
    void RestoreD3DResources();
    void ReleaseD3DResources();
    void PopulateCommandList();
    void WaitForGpu();
    void MoveToNextFrame();
};
