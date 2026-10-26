#define WIN32_LEAN_AND_MEAN
#define NOMINMAX
#include <windows.h>
#include <d3d12.h>
#include <dxgi1_4.h>
#include <d3dcompiler.h>
#include <wrl/client.h>
#include <cstdio>
#include <cstring>
#include <stdexcept>
#include "../src/common/shadow_volume_rules.h"
using Microsoft::WRL::ComPtr;
static void Check(HRESULT result) {if(FAILED(result))throw std::runtime_error("D3D12 test operation failed");}
static ComPtr<ID3DBlob> Shader(const char* source,const char* target) {
    ComPtr<ID3DBlob> result,error;
    const auto hr=D3DCompile(source,std::strlen(source),nullptr,nullptr,nullptr,"main",target,0,0,&result,&error);
    if(FAILED(hr) && error)std::fprintf(stderr,"%s",static_cast<const char*>(error->GetBufferPointer()));
    Check(hr);return result;
}
static unsigned Render(bool clip) {
    ComPtr<IDXGIFactory4> factory;Check(CreateDXGIFactory1(IID_PPV_ARGS(&factory)));
    ComPtr<IDXGIAdapter> adapter;Check(factory->EnumWarpAdapter(IID_PPV_ARGS(&adapter)));
    ComPtr<ID3D12Device> device;Check(D3D12CreateDevice(adapter.Get(),D3D_FEATURE_LEVEL_11_0,IID_PPV_ARGS(&device)));
    D3D12_COMMAND_QUEUE_DESC queue_desc{};queue_desc.Type=D3D12_COMMAND_LIST_TYPE_DIRECT;
    ComPtr<ID3D12CommandQueue> queue;Check(device->CreateCommandQueue(&queue_desc,IID_PPV_ARGS(&queue)));
    ComPtr<ID3D12CommandAllocator> allocator;Check(device->CreateCommandAllocator(D3D12_COMMAND_LIST_TYPE_DIRECT,IID_PPV_ARGS(&allocator)));
    ComPtr<ID3D12GraphicsCommandList> commands;Check(device->CreateCommandList(0,D3D12_COMMAND_LIST_TYPE_DIRECT,allocator.Get(),nullptr,IID_PPV_ARGS(&commands)));
    D3D12_ROOT_SIGNATURE_DESC root_desc{};
    ComPtr<ID3DBlob> root_blob,error;Check(D3D12SerializeRootSignature(&root_desc,D3D_ROOT_SIGNATURE_VERSION_1,&root_blob,&error));
    ComPtr<ID3D12RootSignature> root;Check(device->CreateRootSignature(0,root_blob->GetBufferPointer(),root_blob->GetBufferSize(),IID_PPV_ARGS(&root)));
    const auto volume=Shader("float4 main(uint i:SV_VertexID):SV_Position {float2 p[3]={float2(-1,-1),float2(-1,3),float2(3,-1)};return float4(p[i],1.25,1);}","vs_5_0");
    const auto screen=Shader("float4 main(uint i:SV_VertexID):SV_Position {float2 p[3]={float2(-1,-1),float2(-1,3),float2(3,-1)};return float4(p[i],0.25,1);}","vs_5_0");
    const auto pixel=Shader("float4 main():SV_Target{return float4(1,1,1,1);}","ps_5_0");
    D3D12_GRAPHICS_PIPELINE_STATE_DESC pso{};pso.pRootSignature=root.Get();
    pso.VS={volume->GetBufferPointer(),volume->GetBufferSize()};pso.PS={pixel->GetBufferPointer(),pixel->GetBufferSize()};
    pso.SampleMask=UINT_MAX;pso.SampleDesc.Count=1;pso.NumRenderTargets=1;
    pso.RTVFormats[0]=DXGI_FORMAT_R8G8B8A8_UNORM;pso.DSVFormat=DXGI_FORMAT_D24_UNORM_S8_UINT;
    pso.PrimitiveTopologyType=D3D12_PRIMITIVE_TOPOLOGY_TYPE_TRIANGLE;
    pso.RasterizerState.FillMode=D3D12_FILL_MODE_SOLID;pso.RasterizerState.CullMode=D3D12_CULL_MODE_NONE;
    pso.RasterizerState.DepthClipEnable=clip;
    auto& ds=pso.DepthStencilState;ds.DepthEnable=TRUE;ds.DepthWriteMask=D3D12_DEPTH_WRITE_MASK_ZERO;
    ds.DepthFunc=D3D12_COMPARISON_FUNC_LESS_EQUAL;ds.StencilEnable=TRUE;ds.StencilReadMask=ds.StencilWriteMask=255;
    ds.FrontFace={D3D12_STENCIL_OP_KEEP,D3D12_STENCIL_OP_REPLACE,D3D12_STENCIL_OP_KEEP,D3D12_COMPARISON_FUNC_ALWAYS};
    ds.BackFace=ds.FrontFace;
    auto& blend=pso.BlendState.RenderTarget[0];blend.SrcBlend=blend.SrcBlendAlpha=D3D12_BLEND_ONE;
    blend.DestBlend=blend.DestBlendAlpha=D3D12_BLEND_ZERO;blend.BlendOp=blend.BlendOpAlpha=D3D12_BLEND_OP_ADD;
    ComPtr<ID3D12PipelineState> volume_pso;Check(device->CreateGraphicsPipelineState(&pso,IID_PPV_ARGS(&volume_pso)));
    pso.VS={screen->GetBufferPointer(),screen->GetBufferSize()};pso.RasterizerState.DepthClipEnable=TRUE;
    blend.RenderTargetWriteMask=15;ds.DepthEnable=FALSE;ds.StencilWriteMask=0;
    ds.FrontFace={D3D12_STENCIL_OP_KEEP,D3D12_STENCIL_OP_KEEP,D3D12_STENCIL_OP_KEEP,D3D12_COMPARISON_FUNC_EQUAL};ds.BackFace=ds.FrontFace;
    ComPtr<ID3D12PipelineState> composite_pso;Check(device->CreateGraphicsPipelineState(&pso,IID_PPV_ARGS(&composite_pso)));
    D3D12_HEAP_PROPERTIES gpu_heap{};gpu_heap.Type=D3D12_HEAP_TYPE_DEFAULT;
    D3D12_RESOURCE_DESC tex{};tex.Dimension=D3D12_RESOURCE_DIMENSION_TEXTURE2D;tex.Width=tex.Height=64;
    tex.DepthOrArraySize=tex.MipLevels=1;tex.SampleDesc.Count=1;tex.Format=DXGI_FORMAT_R8G8B8A8_UNORM;tex.Flags=D3D12_RESOURCE_FLAG_ALLOW_RENDER_TARGET;
    D3D12_CLEAR_VALUE clear{};clear.Format=tex.Format;
    ComPtr<ID3D12Resource> colour;Check(device->CreateCommittedResource(&gpu_heap,D3D12_HEAP_FLAG_NONE,&tex,D3D12_RESOURCE_STATE_RENDER_TARGET,&clear,IID_PPV_ARGS(&colour)));
    tex.Format=DXGI_FORMAT_D24_UNORM_S8_UINT;tex.Flags=D3D12_RESOURCE_FLAG_ALLOW_DEPTH_STENCIL;
    clear.Format=tex.Format;clear.DepthStencil={.5f,0};
    ComPtr<ID3D12Resource> depth;Check(device->CreateCommittedResource(&gpu_heap,D3D12_HEAP_FLAG_NONE,&tex,D3D12_RESOURCE_STATE_DEPTH_WRITE,&clear,IID_PPV_ARGS(&depth)));
    D3D12_DESCRIPTOR_HEAP_DESC heap_desc{};heap_desc.NumDescriptors=1;heap_desc.Type=D3D12_DESCRIPTOR_HEAP_TYPE_RTV;
    ComPtr<ID3D12DescriptorHeap> rtv;Check(device->CreateDescriptorHeap(&heap_desc,IID_PPV_ARGS(&rtv)));
    heap_desc.Type=D3D12_DESCRIPTOR_HEAP_TYPE_DSV;
    ComPtr<ID3D12DescriptorHeap> dsv;Check(device->CreateDescriptorHeap(&heap_desc,IID_PPV_ARGS(&dsv)));
    const auto rh=rtv->GetCPUDescriptorHandleForHeapStart(),dh=dsv->GetCPUDescriptorHandleForHeapStart();
    device->CreateRenderTargetView(colour.Get(),nullptr,rh);device->CreateDepthStencilView(depth.Get(),nullptr,dh);
    const float black[4]{};commands->ClearRenderTargetView(rh,black,0,nullptr);
    commands->ClearDepthStencilView(dh,D3D12_CLEAR_FLAGS(D3D12_CLEAR_FLAG_DEPTH|D3D12_CLEAR_FLAG_STENCIL),.5f,0,0,nullptr);
    commands->OMSetRenderTargets(1,&rh,FALSE,&dh);commands->OMSetStencilRef(1);
    const D3D12_VIEWPORT viewport{0,0,64,64,0,1};const D3D12_RECT scissor{0,0,64,64};
    commands->RSSetViewports(1,&viewport);commands->RSSetScissorRects(1,&scissor);
    commands->SetGraphicsRootSignature(root.Get());commands->IASetPrimitiveTopology(D3D_PRIMITIVE_TOPOLOGY_TRIANGLELIST);
    commands->SetPipelineState(volume_pso.Get());commands->DrawInstanced(3,1,0,0);
    commands->SetPipelineState(composite_pso.Get());commands->DrawInstanced(3,1,0,0);
    const auto colour_desc=colour->GetDesc();D3D12_PLACED_SUBRESOURCE_FOOTPRINT footprint{};UINT64 bytes=0;
    device->GetCopyableFootprints(&colour_desc,0,1,0,&footprint,nullptr,nullptr,&bytes);
    D3D12_RESOURCE_DESC buffer{};buffer.Dimension=D3D12_RESOURCE_DIMENSION_BUFFER;buffer.Width=bytes;
    buffer.Height=buffer.DepthOrArraySize=buffer.MipLevels=1;buffer.SampleDesc.Count=1;buffer.Layout=D3D12_TEXTURE_LAYOUT_ROW_MAJOR;
    D3D12_HEAP_PROPERTIES cpu_heap{};cpu_heap.Type=D3D12_HEAP_TYPE_READBACK;
    ComPtr<ID3D12Resource> readback;Check(device->CreateCommittedResource(&cpu_heap,D3D12_HEAP_FLAG_NONE,&buffer,D3D12_RESOURCE_STATE_COPY_DEST,nullptr,IID_PPV_ARGS(&readback)));
    D3D12_RESOURCE_BARRIER barrier{};barrier.Type=D3D12_RESOURCE_BARRIER_TYPE_TRANSITION;
    barrier.Transition={colour.Get(),D3D12_RESOURCE_BARRIER_ALL_SUBRESOURCES,D3D12_RESOURCE_STATE_RENDER_TARGET,D3D12_RESOURCE_STATE_COPY_SOURCE};commands->ResourceBarrier(1,&barrier);
    D3D12_TEXTURE_COPY_LOCATION from{},to{};from.pResource=colour.Get();from.Type=D3D12_TEXTURE_COPY_TYPE_SUBRESOURCE_INDEX;
    to.pResource=readback.Get();to.Type=D3D12_TEXTURE_COPY_TYPE_PLACED_FOOTPRINT;to.PlacedFootprint=footprint;
    commands->CopyTextureRegion(&to,0,0,0,&from,nullptr);Check(commands->Close());
    ID3D12CommandList* lists[]{commands.Get()};queue->ExecuteCommandLists(1,lists);
    ComPtr<ID3D12Fence> fence;Check(device->CreateFence(0,D3D12_FENCE_FLAG_NONE,IID_PPV_ARGS(&fence)));Check(queue->Signal(fence.Get(),1));
    HANDLE done=CreateEventW(nullptr,FALSE,FALSE,nullptr);if(!done)throw std::runtime_error("No fence event");
    Check(fence->SetEventOnCompletion(1,done));const auto wait=WaitForSingleObject(done,30000);CloseHandle(done);
    if(wait!=WAIT_OBJECT_0)throw std::runtime_error("GPU test timeout");
    void* mapped=nullptr;Check(readback->Map(0,nullptr,&mapped));const auto* pixels=static_cast<const uint8_t*>(mapped);
    unsigned marked=0;for(unsigned y=0;y<64;++y)for(unsigned x=0;x<64;++x)if(pixels[y*footprint.Footprint.RowPitch+x*4])++marked;
    readback->Unmap(0,nullptr);return marked;
}
int main() {
    try {
        if(cod_graphics::DepthClip(true,0,false,true,true) ||
           !cod_graphics::DepthClip(false,0,false,true,true) ||
           !cod_graphics::DepthClip(true,15,false,true,true) ||
           !cod_graphics::DepthClip(true,0,true,true,true) ||
           !cod_graphics::DepthClip(true,0,false,false,true) ||
           !cod_graphics::DepthClip(true,0,false,true,false))return 1;
        const auto clipped=Render(true),clamped=Render(cod_graphics::DepthClip(true,0,false,true,true));
        std::printf("D3D12 WARP shadow volume: far-plane clipped=%u, compatible stencil pixels=%u\n",clipped,clamped);
        return clipped==0 && clamped==4096 ? 0:2;
    } catch(const std::exception& e) {std::fprintf(stderr,"%s\n",e.what());return 3;}
}
