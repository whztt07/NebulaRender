//========================================================================
// Copyright (c) Yann Clotioloman Yeo, 2018
//
//	Author					: Yann Clotioloman Yeo
//	E-Mail					: nebularender@gmail.com
//========================================================================

#pragma once

#include "HandleTypes.h"
#include "Descriptor/Allocators.h"
#include "Graphics/Model/TModel.h"
#include "Graphics/Renderer/Realtime/Dx12/Effect/CubeMapping.h"
#include "Graphics/Renderer/Realtime/Dx12/Effect/ForwardLighning.h"
#include "Graphics/Renderer/Realtime/Dx12/Effect/HighlightColor.h"
#include "Graphics/Renderer/Realtime/Dx12/Effect/LightVisualLines.h"
#include "Graphics/Renderer/Realtime/Dx12/Effect/RenderLights.h"
#include "Graphics/Renderer/Realtime/Dx12/Effect/RenderWorldPosition.h"
#include "Graphics/Renderer/Realtime/Dx12/Effect/RenderMoveGizmo.h"
#include "Graphics/Renderer/Realtime/Dx12/Effect/RenderRotationGizmo.h"
#include "Graphics/Renderer/Realtime/Dx12/Effect/RenderScaleGizmo.h"
#include "Graphics/Renderer/Realtime/TRealtimeRenderer.h"

#include <dxgi1_4.h>
#include "Effect/MeshGroupConstantBuffer.h"
#include <atomic>

namespace Graphics { namespace Renderer { namespace Realtime { namespace Dx12
{
struct InitArgs
{
	HWND hwnd;
};

using DX12Model = Graphics::Model::TModel<DX12_GRAPHIC_ALLOC_PARAMETERS>;

class DX12Renderer : public TRealtimeRenderer<DX12_GRAPHIC_ALLOC_PARAMETERS, InitArgs>
{
public:
	nbBool init(const InitArgs& args) override;
	void finishTasks() override;
	void release() override;

	Model::ModelPtr createModelFromRaw(const std::string& path) const override;
	Model::ModelPtr createModelFromNode(const Utilities::Xml::XmlNode& node) const override;

	Texture::CubeMapPtr createCubeMapFromRaw(Graphics::Texture::CubeMapConstructRawArgs& args) const override;
	Texture::CubeMapPtr createCubeMapFromNode(const Graphics::Texture::CubeMapConstructNodeArgs& args) const override;

	Scene::GizmoMap createGizmos() const override;

	void drawScene(const Scene::BaseScene& scene) override;
	void present() override;

	void createTexture2D(const Texture::Image* image, Dx12TextureHandle& dst) override;
	void createTextureCube(const std::vector<const Texture::Image*>& images, Dx12TextureHandle& dst) override;

	void releaseTexture(const Dx12TextureHandle& textureHandle) const override;

	nbBool createVertexBuffer(const void* data, Dx12VertexBufferHandle& dst, nbUint32 sizeofElem, nbUint32 count) override;
	void releaseVertexBuffer(const Dx12VertexBufferHandle& arrayBufferHandle) const override;

	nbBool createIndexBuffer(const std::vector<nbUint32>& data, Dx12IndexBufferHandle& dst) override;
	void releaseIndexBuffer(const Dx12IndexBufferHandle& arrayBufferHandle) const override;

	void resizeBuffers(const glm::uvec2& newSize) override;
	void onMaterialChanged(const Scene::BaseScene& scene, const MaterialIdentifier& matId) override;
	void onGroupTransformChanged(const Scene::BaseScene& scene, const Model::MeshGroupId& groupId) override;

	void updateLightBuffers(const Scene::BaseScene& scene) override;
	void updateMaterialBuffers(const Scene::BaseScene& scene) override;

	IntersectionInfo queryIntersection(const Scene::BaseScene& scene, const glm::uvec2& pos) override;

	void startCommandRecording() override;
	void endSceneLoadCommandRecording(const Scene::BaseScene* scene) override;
	void endCommandRecording() override;

	void tryWaitEffectCompilationDone();

private:
	enum class CommandType
	{
		Direct,
		Copy
	};

	void startCommandRecording(CommandType commandType);
	void endCommandRecording(CommandType commandType);
	void waitCommandsFinish(CommandType commandType, nbInt32 frameIdx);
	void waitCurrentFrameCommandsFinish(CommandType commandType);

	void prepareViewportRender();

	// Dx12 initilization helpers
	// @See: // https://www.braynzarsoft.net/viewtutorial/q16390-03-initializing-directx-12
	nbBool createDevice(IDXGIFactory4* m_dxgiFactory);
	nbBool createCommandQueues();
	nbBool createSwapChain(IDXGIFactory4* m_dxgiFactory, const glm::uvec2& bufferSize);
	nbBool createRenderTargets(const glm::uvec2& bufferSize);
	nbBool createDepthStencilBuffers(const glm::uvec2& bufferSize);
	void setUpViewportAndScissor(const glm::uvec2& bufferSize);
	nbBool createPixelReadBuffer();
	nbBool bindSwapChainRenderTargets();
	nbBool createCommandAllocators();
	nbBool createCommandLists();
	nbBool createFencesAndFenceEvent();
	void releaseSwapChainDynamicResources();

	struct ArrayBufferResource : ResourceBuffer
	{
		nbUint32 bufferSize = 0u;
	};

	ArrayBufferResource createArrayBufferRecource(const void* data, nbUint32 sizeofElem, nbUint32 count);

	void createTexture2DArray(const std::vector<const Texture::Image*>& images, nbUint32 width, nbUint32 height, D3D12_SRV_DIMENSION viewDimension, Dx12TextureHandle& dst);

	// The window
	HWND m_hwindow;

	// Descriptor allocators
	std::unique_ptr<Descriptor::CBV_SRV_UAV_DescriptorAllocator> m_cbs_srv_uavAllocator;
	std::unique_ptr<Descriptor::RTV_DescriptorAllocator> m_rtvAllocator;
	std::unique_ptr<Descriptor::DSV_DescriptorAllocator> m_dsvAllocator;

	CComPtr<IDXGIFactory4> m_dxgiFactory;
	CComPtr<IDXGISwapChain3> m_swapChain;
	DXGI_SWAP_CHAIN_DESC m_swapChainDesc = {};
	ID3D12Resource* m_renderTargets[SwapChainBufferCount];
	CComPtr<ID3D12Resource> m_pixelReadBuffer;

	struct CommandBuffer
	{
		CComPtr<ID3D12CommandAllocator> commandAllocator[SwapChainBufferCount];
		CComPtr<ID3D12GraphicsCommandList> commandList;
		CComPtr<ID3D12CommandQueue> commandQueue;
		CComPtr<ID3D12Fence> fences[SwapChainBufferCount];
		HANDLE fenceEvent;
		UINT64 fenceValue[SwapChainBufferCount];
		nbInt32 frameIndex;
	};

	std::unordered_map<CommandType, CommandBuffer> m_commandBuffers;

	D3D12_VIEWPORT m_viewport; 
	D3D12_RECT m_scissorRect; 

	DXGI_SAMPLE_DESC m_msaaSampleDesc = {};
	DXGI_SAMPLE_DESC m_simpleSampleDesc = {};

	ID3D12Resource* m_depthStencilBuffer =  nullptr;
	DescriptorHandle m_msaaDsvDescriptorHandle;

	// The offscreen render targets
	ID3D12Resource* m_msaaRenderTarget = nullptr;
	DescriptorHandle m_msaaRTDescriptorHandle;

	ID3D12Resource* m_positionRenderTarget = nullptr;
	DescriptorHandle m_positionRTDescriptorHandle;
	ID3D12Resource* m_positionDepthStencilBuffer = nullptr;
	DescriptorHandle m_positionDsvDescriptorHandle;

	std::vector<ID3D12Resource*> m_loadingResources;

	std::unique_ptr<Effect::ForwardLighning> m_forwardLightningEffect;
	std::unique_ptr<Effect::CubeMapping> m_cubeMappingEffect;
	std::unique_ptr<Effect::HighlightColor> m_highlightColorEffect;
	std::unique_ptr<Effect::RenderLights> m_renderLightsEffect;
	std::unique_ptr<Effect::RenderMoveGizmo> m_moveGizmoEffect;
	std::unique_ptr<Effect::RenderScaleGizmo> m_scaleGizmoEffect;
	std::unique_ptr<Effect::RenderRotationGizmo> m_rotationGizmoEffect;
	std::unique_ptr<Effect::LightVisualLines> m_lightVisualLinesEffect;
	std::unique_ptr<Effect::RenderWorldPosition> m_renderPositionEffect;

	std::thread m_compileEffectThread;
	std::atomic_bool m_effectsReady;
};

inline Model::ModelPtr DX12Renderer::createModelFromRaw(const std::string& path) const
{
	return std::make_unique<DX12Model>(path);
}

inline Model::ModelPtr DX12Renderer::createModelFromNode(const Utilities::Xml::XmlNode& node)const
{
	return std::make_unique<DX12Model>(node);
}

inline Texture::CubeMapPtr DX12Renderer::createCubeMapFromRaw(Texture::CubeMapConstructRawArgs& args) const
{
	return std::make_unique<DX12CubeMap>(args);
}
inline Texture::CubeMapPtr DX12Renderer::createCubeMapFromNode(const Texture::CubeMapConstructNodeArgs& args) const
{
	return std::make_unique<DX12CubeMap>(args);
}

inline nbBool DX12Renderer::createVertexBuffer(const void* data, Dx12VertexBufferHandle& dst, nbUint32 sizeofElem, nbUint32 count)
{
	const ArrayBufferResource resourceData = createArrayBufferRecource(data, sizeofElem, count);
	if (resourceData.bufferSize == 0u || resourceData.buffer == nullptr)
	{
		return false;
	}

	dst.buffer = resourceData.buffer;
	dst.count = count;
	dst.bufferView.BufferLocation = dst.buffer->GetGPUVirtualAddress();
	dst.bufferView.StrideInBytes = sizeofElem;
	dst.bufferView.SizeInBytes = resourceData.bufferSize;

	return true;
}

inline void DX12Renderer::onMaterialChanged(const Scene::BaseScene& scene, const MaterialIdentifier& matId)
{
	m_forwardLightningEffect->onUpdateMaterial(scene, matId, m_commandBuffers[CommandType::Direct].commandList);
}

inline void DX12Renderer::onGroupTransformChanged(const Scene::BaseScene& scene, const Model::MeshGroupId& groupId)
{
	Effect::MeshGroupConstantBufferSingleton::instance()->onUpdateMeshGroup(scene, groupId, m_commandBuffers[CommandType::Direct].commandList);
}

inline void DX12Renderer::updateMaterialBuffers(const Scene::BaseScene& scene)
{
	m_forwardLightningEffect->updateMaterialBuffers(scene, m_commandBuffers[CommandType::Direct].commandList);
}

inline void DX12Renderer::updateLightBuffers(const Scene::BaseScene& scene)
{
	m_renderLightsEffect->updateLightBuffers(scene);
}

inline void DX12Renderer::createTexture2D(const Texture::Image* image, Dx12TextureHandle& dst)
{
	createTexture2DArray({ image }, image->getWidth(), image->getHeight(), D3D12_SRV_DIMENSION_TEXTURE2D, dst);
}

inline void DX12Renderer::releaseTexture(const Dx12TextureHandle& textureHandle) const
{
	m_cbs_srv_uavAllocator->release(textureHandle.descriptorHandle);
	textureHandle.buffer->Release();
}

inline void DX12Renderer::releaseVertexBuffer(const Dx12VertexBufferHandle& arrayBufferHandle) const
{
	arrayBufferHandle.buffer->Release();
}

inline void DX12Renderer::releaseIndexBuffer(const Dx12IndexBufferHandle& arrayBufferHandle) const
{
	arrayBufferHandle.buffer->Release();
}

}}}}
