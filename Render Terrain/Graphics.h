/*
Graphics.h

Author:			Chris Serson
Last Edited:	August 1, 2016

Description:	Class for creating and managing a Direct3D 12 instance.

Usage:			- Calling the constructor, either through Graphics GFX(...);
				or Graphics* GFX; GFX = new Graphics(...);, will find a
				Direct3D 12 compatible hardware device and initialize
				Direct3D on it.
				- Proper shutdown is handled by the destructor.
				- All requests to the graphics device must go through the
				Graphics object. This includes RTVs, SRVs, and PSOs.
				- The calling object can request a Command List from the Graphics
				object (GetCommandList()). Currently only supports 1 thread.
				- The calling object must tell the Graphics object when to
				reset the pipeline for a new frame (ResetPipeline()), 
				when to swap the buffers (SetBackBufferRender(), SetBackBufferPresent(), 
				and when to actually execute the command list (Render()).

Future Work:	- Add support for multi-threaded graphics applications.
				- Clean up constructor code. Can be broken up into smaller
				functions to make more readable.
				- Needs to be more generic, with less hard-coded values.
*/
#pragma once

// Linkages
#pragma comment(lib, "d3d12.lib")
#pragma comment(lib, "dxgi.lib")
#pragma comment(lib, "d3dcompiler.lib")

#include "D3DX12.h"
#include <dxgi1_5.h>
#include <DirectXMath.h>
#include <D3DCompiler.h>
#include <stdexcept>

namespace graphics {
	using namespace DirectX;

	static const float SCREEN_DEPTH = 1000.0f;
	static const float SCREEN_NEAR = 0.1f;
	static const UINT FACTORY_DEBUG = DXGI_CREATE_FACTORY_DEBUG; // set to 0 if not debugging, DXGI_CREATE_FACTORY_DEBUG if debugging.
	static const DXGI_FORMAT DESIRED_FORMAT = DXGI_FORMAT_R8G8B8A8_UNORM;
	static const int FRAME_BUFFER_COUNT = 3; // triple buffering.
	static const D3D_FEATURE_LEVEL	FEATURE_LEVEL = D3D_FEATURE_LEVEL_11_0; // minimum feature level necessary for DirectX 12 compatibility.
																			// this is all my current card supports.
	enum ShaderType { PIXEL_SHADER, VERTEX_SHADER, GEOMETRY_SHADER, HULL_SHADER, DOMAIN_SHADER };

	class GFX_Exception : public std::runtime_error {
	public:
		GFX_Exception(const char *msg) : std::runtime_error(msg) {}
	};
	
	class Graphics	{
	public:
		Graphics(int height, int width, HWND win, bool fullscreen);
		~Graphics();

		// send command list to graphics card for rendering.
		void Render();
		void Run();
		// reset the pipeline for the next frame.
		void ResetPipeline();
		// set the back buffer as the render target for the provided command list.
		void SetBackBufferRender(ID3D12GraphicsCommandList* cmdList, const float clearColor[4]);
		// set the back buffer as presenting for the provided command list.
		void SetBackBufferPresent(ID3D12GraphicsCommandList* cmdList);

		// Returns a pointer to the command list.
		ID3D12GraphicsCommandList* GetCommandList() { return mpCmdList; }

		UINT GetDescriptorHandleIncrementSize(D3D12_DESCRIPTOR_HEAP_TYPE ht);

		// Create and return a pointer to a new root signature matching the provided description.
		void CreateRootSig(CD3DX12_ROOT_SIGNATURE_DESC* rootDesc, ID3D12RootSignature*& rootSig);
		// Create and return a pointer to a new Pipeline State Object matching the provided description.
		void CreatePSO(D3D12_GRAPHICS_PIPELINE_STATE_DESC* psoDesc, ID3D12PipelineState*& PSO);
		// Create and return a pointer to a Descriptor Heap.
		void CreateDescriptorHeap(D3D12_DESCRIPTOR_HEAP_DESC* heapDesc, ID3D12DescriptorHeap*& heap);
		// Create a Shader Resource view for the supplied resource.
		void CreateSRV(ID3D12Resource*& tex, D3D12_SHADER_RESOURCE_VIEW_DESC* srvDesc, D3D12_CPU_DESCRIPTOR_HANDLE handle);
		// Create a constant buffer view
		void CreateCBV(D3D12_CONSTANT_BUFFER_VIEW_DESC* desc, D3D12_CPU_DESCRIPTOR_HANDLE handle);
		// Create a depth/stencil buffer view
		void CreateDSV(ID3D12Resource*& tex, D3D12_DEPTH_STENCIL_VIEW_DESC* desc, D3D12_CPU_DESCRIPTOR_HANDLE handle);
		// Create an upload buffer, ready for mapping.
		void CreateUploadBuffer(ID3D12Resource*& buffer, D3D12_RESOURCE_DESC* texDesc);
		// Create a default buffer, preconfigured as a copy destination
		void CreateDefaultBuffer(ID3D12Resource*& buffer, D3D12_RESOURCE_DESC* texDesc);
		// Create a commited resource. More general version of CreateUploadBuffer and CreateDefaultBuffer. Still needs work/testing.
		void CreateCommittedResource(ID3D12Resource*& heap, D3D12_RESOURCE_DESC* descTex, D3D12_HEAP_PROPERTIES* propHeap, D3D12_HEAP_FLAGS flags, D3D12_RESOURCE_STATES state, D3D12_CLEAR_VALUE* clear);
		// Compile the specified shader.
		void CompileShader(LPCWSTR filename, D3D12_SHADER_BYTECODE& shaderBytecode, ShaderType st);
		// Waits for and confirms that the GPU is done running any commands on the current back buffer.
		void LoadAssets();
		// Function to be called before shutting down. Ensures GPU is done rendering all frames so we can release graphics resources.
		void ClearAllFrames();

	private:
		void NextFrame();
		
		ID3D12Device*				mpDev;
		ID3D12CommandQueue*			mpCmdQ;
		ID3D12CommandAllocator*		maCmdAllocators[FRAME_BUFFER_COUNT];
		ID3D12GraphicsCommandList*	mpCmdList;
		IDXGISwapChain3*			mpSwapChain;
		ID3D12DescriptorHeap*		mpRTVHeap; // Render Target View Heap
		ID3D12DescriptorHeap*		mpDSVHeap; // Depth Stencil View Heap
		ID3D12Resource*				maBackBuffers[FRAME_BUFFER_COUNT];
		ID3D12Resource*				mpDepthStencilBuffer;
		ID3D12Fence*				maFences[FRAME_BUFFER_COUNT];
		HANDLE						mFenceEvent;
		unsigned long long			maFenceValues[FRAME_BUFFER_COUNT];
		unsigned int				mBufferIndex;
		unsigned int				mRTVDescSize; // Descriptor sizes may vary from device to device, so keep the size around so we can increment an offset when necessary.
	};
};
