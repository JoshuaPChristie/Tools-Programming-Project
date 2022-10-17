//
// Game.cpp
//

#include "pch.h"
#include "Game.h"
#include "DisplayObject.h"
#include <string>


using namespace DirectX;
using namespace DirectX::SimpleMath;

using Microsoft::WRL::ComPtr;

Game::Game()

{
    m_deviceResources = std::make_unique<DX::DeviceResources>();
    m_deviceResources->RegisterDeviceNotify(this);
	m_displayList.clear();
	
	//initial Settings
	//modes
	m_grid = false;

	m_camera = Camera::Camera();

}

Game::~Game()
{

#ifdef DXTK_AUDIO
    if (m_audEngine)
    {
        m_audEngine->Suspend();
    }
#endif
}

// Initialize the Direct3D resources required to run.
void Game::Initialize(HWND window, int width, int height)
{
    m_gamePad = std::make_unique<GamePad>();

    m_keyboard = std::make_unique<Keyboard>();

    m_mouse = std::make_unique<Mouse>();
    m_mouse->SetWindow(window);

    m_deviceResources->SetWindow(window, width, height);

    m_deviceResources->CreateDeviceResources();
    CreateDeviceDependentResources();

    m_deviceResources->CreateWindowSizeDependentResources();
    CreateWindowSizeDependentResources();

	//additional
	GetClientRect(window, &m_ScreenDimensions);

#ifdef DXTK_AUDIO
    // Create DirectXTK for Audio objects
    AUDIO_ENGINE_FLAGS eflags = AudioEngine_Default;
#ifdef _DEBUG
    eflags = eflags | AudioEngine_Debug;
#endif

    m_audEngine = std::make_unique<AudioEngine>(eflags);

    m_audioEvent = 0;
    m_audioTimerAcc = 10.f;
    m_retryDefault = false;

    m_waveBank = std::make_unique<WaveBank>(m_audEngine.get(), L"adpcmdroid.xwb");

    m_soundEffect = std::make_unique<SoundEffect>(m_audEngine.get(), L"MusicMono_adpcm.wav");
    m_effect1 = m_soundEffect->CreateInstance();
    m_effect2 = m_waveBank->CreateInstance(10);

    m_effect1->Play(true);
    m_effect2->Play();
#endif
}

void Game::SetGridState(bool state)
{
	m_grid = state;
}

#pragma region Frame Update
// Executes the basic game loop.
void Game::Tick(InputCommands *Input)
{
	//copy over the input commands so we have a local version to use elsewhere.
	m_InputCommands = *Input;
    m_timer.Tick([&]()
    {
        Update(m_timer);
    });

#ifdef DXTK_AUDIO
    // Only update audio engine once per frame
    if (!m_audEngine->IsCriticalError() && m_audEngine->Update())
    {
        // Setup a retry in 1 second
        m_audioTimerAcc = 1.f;
        m_retryDefault = true;
    }
#endif

    Render();
}

// Updates the world.
void Game::Update(DX::StepTimer const& timer)
{
	m_camera.Update(m_InputCommands);

	//apply camera vectors
    m_view = Matrix::CreateLookAt(m_camera.getPosition(), m_camera.getLookAt(), Vector3::UnitY);

    m_batchEffect->SetView(m_view);
    m_batchEffect->SetWorld(Matrix::Identity);
	m_displayChunk.m_terrainEffect->SetView(m_view);
	m_displayChunk.m_terrainEffect->SetWorld(Matrix::Identity);

#ifdef DXTK_AUDIO
    m_audioTimerAcc -= (float)timer.GetElapsedSeconds();
    if (m_audioTimerAcc < 0)
    {
        if (m_retryDefault)
        {
            m_retryDefault = false;
            if (m_audEngine->Reset())
            {
                // Restart looping audio
                m_effect1->Play(true);
            }
        }
        else
        {
            m_audioTimerAcc = 4.f;

            m_waveBank->Play(m_audioEvent++);

            if (m_audioEvent >= 11)
                m_audioEvent = 0;
        }
    }
#endif

   
}
#pragma endregion

#pragma region Frame Render
// Draws the scene.
void Game::Render()
{
    // Don't try to render anything before the first Update.
    if (m_timer.GetFrameCount() == 0)
    {
        return;
    }

    Clear();

    m_deviceResources->PIXBeginEvent(L"Render");
    auto context = m_deviceResources->GetD3DDeviceContext();

	if (m_grid)
	{
		// Draw procedurally generated dynamic grid
		const XMVECTORF32 xaxis = { 512.f, 0.f, 0.f };
		const XMVECTORF32 yaxis = { 0.f, 0.f, 512.f };
		DrawGrid(xaxis, yaxis, g_XMZero, 512, 512, Colors::Gray);
	}
	//CAMERA POSITION ON HUD
	m_sprites->Begin();
	WCHAR   Buffer[256];
	std::wstring var = L"Cam X: " + std::to_wstring(m_camPosition.x) + L"Cam Z: " + std::to_wstring(m_camPosition.z);
	m_font->DrawString(m_sprites.get(), var.c_str() , XMFLOAT2(100, 10), Colors::Yellow);
	m_sprites->End();

	//RENDER OBJECTS FROM SCENEGRAPH
	int numRenderObjects = m_displayList.size();
	for (int i = 0; i < numRenderObjects; i++)
	{
		m_deviceResources->PIXBeginEvent(L"Draw model");
		const XMVECTORF32 scale = { m_displayList[i].m_scale.x, m_displayList[i].m_scale.y, m_displayList[i].m_scale.z };
		const XMVECTORF32 translate = { m_displayList[i].m_position.x, m_displayList[i].m_position.y, m_displayList[i].m_position.z };

		//convert degrees into radians for rotation matrix
		XMVECTOR rotate = Quaternion::CreateFromYawPitchRoll(m_displayList[i].m_orientation.y *3.1415 / 180,
															m_displayList[i].m_orientation.x *3.1415 / 180,
															m_displayList[i].m_orientation.z *3.1415 / 180);

		XMMATRIX local = m_world * XMMatrixTransformation(g_XMZero, Quaternion::Identity, scale, g_XMZero, rotate, translate);

		m_displayList[i].m_model->Draw(context, *m_states, local, m_view, m_projection, false);	//last variable in draw,  make TRUE for wireframe

		m_deviceResources->PIXEndEvent();
	}
    m_deviceResources->PIXEndEvent();

	//RENDER TERRAIN
	context->OMSetBlendState(m_states->Opaque(), nullptr, 0xFFFFFFFF);
	context->OMSetDepthStencilState(m_states->DepthDefault(),0);
	context->RSSetState(m_states->CullNone());
//	context->RSSetState(m_states->Wireframe());		//uncomment for wireframe

	//Render the batch,  This is handled in the Display chunk becuase it has the potential to get complex
	m_displayChunk.RenderBatch(m_deviceResources);

    m_deviceResources->Present();
}

// Helper method to clear the back buffers.
void Game::Clear()
{
    m_deviceResources->PIXBeginEvent(L"Clear");

    // Clear the views.
    auto context = m_deviceResources->GetD3DDeviceContext();
    auto renderTarget = m_deviceResources->GetBackBufferRenderTargetView();
    auto depthStencil = m_deviceResources->GetDepthStencilView();

    context->ClearRenderTargetView(renderTarget, Colors::CornflowerBlue);
    context->ClearDepthStencilView(depthStencil, D3D11_CLEAR_DEPTH | D3D11_CLEAR_STENCIL, 1.0f, 0);
    context->OMSetRenderTargets(1, &renderTarget, depthStencil);

    // Set the viewport.
    auto viewport = m_deviceResources->GetScreenViewport();
    context->RSSetViewports(1, &viewport);

    m_deviceResources->PIXEndEvent();
}

void XM_CALLCONV Game::DrawGrid(FXMVECTOR xAxis, FXMVECTOR yAxis, FXMVECTOR origin, size_t xdivs, size_t ydivs, GXMVECTOR color)
{
    m_deviceResources->PIXBeginEvent(L"Draw grid");

    auto context = m_deviceResources->GetD3DDeviceContext();
    context->OMSetBlendState(m_states->Opaque(), nullptr, 0xFFFFFFFF);
    context->OMSetDepthStencilState(m_states->DepthNone(), 0);
    context->RSSetState(m_states->CullCounterClockwise());

    m_batchEffect->Apply(context);

    context->IASetInputLayout(m_batchInputLayout.Get());

    m_batch->Begin();

    xdivs = std::max<size_t>(1, xdivs);
    ydivs = std::max<size_t>(1, ydivs);

    for (size_t i = 0; i <= xdivs; ++i)
    {
        float fPercent = float(i) / float(xdivs);
        fPercent = (fPercent * 2.0f) - 1.0f;
        XMVECTOR vScale = XMVectorScale(xAxis, fPercent);
        vScale = XMVectorAdd(vScale, origin);

        VertexPositionColor v1(XMVectorSubtract(vScale, yAxis), color);
        VertexPositionColor v2(XMVectorAdd(vScale, yAxis), color);
        m_batch->DrawLine(v1, v2);
    }

    for (size_t i = 0; i <= ydivs; i++)
    {
        float fPercent = float(i) / float(ydivs);
        fPercent = (fPercent * 2.0f) - 1.0f;
        XMVECTOR vScale = XMVectorScale(yAxis, fPercent);
        vScale = XMVectorAdd(vScale, origin);

        VertexPositionColor v1(XMVectorSubtract(vScale, xAxis), color);
        VertexPositionColor v2(XMVectorAdd(vScale, xAxis), color);
        m_batch->DrawLine(v1, v2);
    }

    m_batch->End();

    m_deviceResources->PIXEndEvent();
}
#pragma endregion

#pragma region Message Handlers
// Message handlers
void Game::OnActivated()
{
}

void Game::OnDeactivated()
{
}

void Game::OnSuspending()
{
#ifdef DXTK_AUDIO
    m_audEngine->Suspend();
#endif
}

void Game::OnResuming()
{
    m_timer.ResetElapsedTime();

#ifdef DXTK_AUDIO
    m_audEngine->Resume();
#endif
}

void Game::OnWindowSizeChanged(int width, int height)
{
    if (!m_deviceResources->WindowSizeChanged(width, height))
        return;

    CreateWindowSizeDependentResources();
}

void Game::BuildDisplayList(std::vector<SceneObject> * SceneGraph)
{
	auto device = m_deviceResources->GetD3DDevice();
	auto devicecontext = m_deviceResources->GetD3DDeviceContext();

	if (!m_displayList.empty())		//is the vector empty
	{
		m_displayList.clear();		//if not, empty it
	}

	//for every item in the scenegraph
	int numObjects = SceneGraph->size();
	for (int i = 0; i < numObjects; i++)
	{
		
		//create a temp display object that we will populate then append to the display list.
		DisplayObject newDisplayObject;
		
		//load model
		newDisplayObject.m_model_path = SceneGraph->at(i).model_path;
		std::wstring modelwstr = StringToWCHART(SceneGraph->at(i).model_path);							//convect string to Wchar
		newDisplayObject.m_model = Model::CreateFromCMO(device, modelwstr.c_str(), *m_fxFactory, true);	//get DXSDK to load model "False" for LH coordinate system (maya)

		//Load Texture
		newDisplayObject.m_tex_diffuse_path = SceneGraph->at(i).tex_diffuse_path;
		std::wstring texturewstr = StringToWCHART(SceneGraph->at(i).tex_diffuse_path);								//convect string to Wchar
		HRESULT rs;
		rs = CreateDDSTextureFromFile(device, texturewstr.c_str(), nullptr, &newDisplayObject.m_texture_diffuse);	//load tex into Shader resource

		//if texture fails.  load error default
		if (rs)
		{
			CreateDDSTextureFromFile(device, L"database/data/Error.dds", nullptr, &newDisplayObject.m_texture_diffuse);	//load tex into Shader resource
		}

		//apply new texture to models effect
		newDisplayObject.m_model->UpdateEffects([&](IEffect* effect) //This uses a Lambda function,  if you dont understand it: Look it up.
		{	
			auto lights = dynamic_cast<BasicEffect*>(effect);
			if (lights)
			{
				lights->SetTexture(newDisplayObject.m_texture_diffuse);			
			}
		});


		newDisplayObject.m_ID = SceneGraph->at(i).ID;

		//set position
		newDisplayObject.m_position.x = SceneGraph->at(i).posX;
		newDisplayObject.m_position.y = SceneGraph->at(i).posY;
		newDisplayObject.m_position.z = SceneGraph->at(i).posZ;
		
		//setorientation
		newDisplayObject.m_orientation.x = SceneGraph->at(i).rotX;
		newDisplayObject.m_orientation.y = SceneGraph->at(i).rotY;
		newDisplayObject.m_orientation.z = SceneGraph->at(i).rotZ;

		//set scale
		newDisplayObject.m_scale.x = SceneGraph->at(i).scaX;
		newDisplayObject.m_scale.y = SceneGraph->at(i).scaY;
		newDisplayObject.m_scale.z = SceneGraph->at(i).scaZ;

		//set wireframe / render flags
		newDisplayObject.m_render		= SceneGraph->at(i).editor_render;
		newDisplayObject.m_wireframe	= SceneGraph->at(i).editor_wireframe;

		newDisplayObject.m_light_type		= SceneGraph->at(i).light_type;
		newDisplayObject.m_light_diffuse_r	= SceneGraph->at(i).light_diffuse_r;
		newDisplayObject.m_light_diffuse_g	= SceneGraph->at(i).light_diffuse_g;
		newDisplayObject.m_light_diffuse_b	= SceneGraph->at(i).light_diffuse_b;
		newDisplayObject.m_light_specular_r = SceneGraph->at(i).light_specular_r;
		newDisplayObject.m_light_specular_g = SceneGraph->at(i).light_specular_g;
		newDisplayObject.m_light_specular_b = SceneGraph->at(i).light_specular_b;
		newDisplayObject.m_light_spot_cutoff = SceneGraph->at(i).light_spot_cutoff;
		newDisplayObject.m_light_constant	= SceneGraph->at(i).light_constant;
		newDisplayObject.m_light_linear		= SceneGraph->at(i).light_linear;
		newDisplayObject.m_light_quadratic	= SceneGraph->at(i).light_quadratic;
		
		m_displayList.push_back(newDisplayObject);
		
	}
		
	spawnedObjects = m_displayList.size();
		
}

void Game::BuildDisplayChunk(ChunkObject * SceneChunk)
{
	//populate our local DISPLAYCHUNK with all the chunk info we need from the object stored in toolmain
	//which, to be honest, is almost all of it. Its mostly rendering related info so...
	m_displayChunk.PopulateChunkData(SceneChunk);		//migrate chunk data
	m_displayChunk.LoadHeightMap(m_deviceResources);
	m_displayChunk.m_terrainEffect->SetProjection(m_projection);
	m_displayChunk.InitialiseBatch();
}

void Game::SaveDisplayChunk(ChunkObject * SceneChunk)
{
	m_displayChunk.SaveHeightMap();			//save heightmap to file.
}

#ifdef DXTK_AUDIO
void Game::NewAudioDevice()
{
    if (m_audEngine && !m_audEngine->IsAudioDevicePresent())
    {
        // Setup a retry in 1 second
        m_audioTimerAcc = 1.f;
        m_retryDefault = true;
    }
}
#endif


#pragma endregion

#pragma region Direct3D Resources
// These are the resources that depend on the device.
void Game::CreateDeviceDependentResources()
{
    auto context = m_deviceResources->GetD3DDeviceContext();
    auto device = m_deviceResources->GetD3DDevice();

    m_states = std::make_unique<CommonStates>(device);

    m_fxFactory = std::make_unique<EffectFactory>(device);
	m_fxFactory->SetDirectory(L"database/data/"); //fx Factory will look in the database directory
	m_fxFactory->SetSharing(false);	//we must set this to false otherwise it will share effects based on the initial tex loaded (When the model loads) rather than what we will change them to.

    m_sprites = std::make_unique<SpriteBatch>(context);

    m_batch = std::make_unique<PrimitiveBatch<VertexPositionColor>>(context);

    m_batchEffect = std::make_unique<BasicEffect>(device);
    m_batchEffect->SetVertexColorEnabled(true);

    {
        void const* shaderByteCode;
        size_t byteCodeLength;

        m_batchEffect->GetVertexShaderBytecode(&shaderByteCode, &byteCodeLength);

        DX::ThrowIfFailed(
            device->CreateInputLayout(VertexPositionColor::InputElements,
                VertexPositionColor::InputElementCount,
                shaderByteCode, byteCodeLength,
                m_batchInputLayout.ReleaseAndGetAddressOf())
        );
    }

    m_font = std::make_unique<SpriteFont>(device, L"SegoeUI_18.spritefont");

//    m_shape = GeometricPrimitive::CreateTeapot(context, 4.f, 8);

    // SDKMESH has to use clockwise winding with right-handed coordinates, so textures are flipped in U
    m_model = Model::CreateFromSDKMESH(device, L"tiny.sdkmesh", *m_fxFactory);
	

    // Load textures
    DX::ThrowIfFailed(
        CreateDDSTextureFromFile(device, L"seafloor.dds", nullptr, m_texture1.ReleaseAndGetAddressOf())
    );

    DX::ThrowIfFailed(
        CreateDDSTextureFromFile(device, L"windowslogo.dds", nullptr, m_texture2.ReleaseAndGetAddressOf())
    );

}

// Allocate all memory resources that change on a window SizeChanged event.
void Game::CreateWindowSizeDependentResources()
{
    auto size = m_deviceResources->GetOutputSize();
    float aspectRatio = float(size.right) / float(size.bottom);
    float fovAngleY = 70.0f * XM_PI / 180.0f;

    // This is a simple example of change that can be made when the app is in
    // portrait or snapped view.
    if (aspectRatio < 1.0f)
    {
        fovAngleY *= 2.0f;
    }

    // This sample makes use of a right-handed coordinate system using row-major matrices.
    m_projection = Matrix::CreatePerspectiveFieldOfView(
        fovAngleY,
        aspectRatio,
        0.01f,
        1000.0f
    );

    m_batchEffect->SetProjection(m_projection);
	
}

void Game::OnDeviceLost()
{
    m_states.reset();
    m_fxFactory.reset();
    m_sprites.reset();
    m_batch.reset();
    m_batchEffect.reset();
    m_font.reset();
    m_shape.reset();
    m_model.reset();
    m_texture1.Reset();
    m_texture2.Reset();
    m_batchInputLayout.Reset();
}

void Game::OnDeviceRestored()
{
    CreateDeviceDependentResources();

    CreateWindowSizeDependentResources();
}
#pragma endregion

std::wstring StringToWCHART(std::string s)
{

	int len;
	int slength = (int)s.length() + 1;
	len = MultiByteToWideChar(CP_ACP, 0, s.c_str(), slength, 0, 0);
	wchar_t* buf = new wchar_t[len];
	MultiByteToWideChar(CP_ACP, 0, s.c_str(), slength, buf, len);
	std::wstring r(buf);
	delete[] buf;
	return r;
}

//additional
int Game::MousePicking()
{
	int selectedID = -1;
	float pickedDistance = 0;
	float closestDistance = 10000;

	//setup near and far planes of frustum with mouse X and mouse y passed down from Toolmain. 
		//they may look the same but note, the difference in Z
	const XMVECTOR nearSource = XMVectorSet(m_InputCommands.mousePosX, m_InputCommands.mousePosY, 0.0f, 1.0f);
	const XMVECTOR farSource = XMVectorSet(m_InputCommands.mousePosX, m_InputCommands.mousePosY, 1.0f, 1.0f);

	//Loop through entire display list of objects and pick with each in turn. 
	for (int i = 0; i < m_displayList.size(); i++)
	{
		//Get the scale factor and translation of the object
		const XMVECTORF32 scale = { m_displayList[i].m_scale.x,		m_displayList[i].m_scale.y,		m_displayList[i].m_scale.z };
		const XMVECTORF32 translate = { m_displayList[i].m_position.x,		m_displayList[i].m_position.y,	m_displayList[i].m_position.z };

		//convert euler angles into a quaternion for the rotation of the object
		XMVECTOR rotate = Quaternion::CreateFromYawPitchRoll(m_displayList[i].m_orientation.y * 3.1415 / 180, m_displayList[i].m_orientation.x * 3.1415 / 180,
			m_displayList[i].m_orientation.z * 3.1415 / 180);

		//create set the matrix of the selected object in the world based on the translation, scale and rotation.
		XMMATRIX local = m_world * XMMatrixTransformation(g_XMZero, Quaternion::Identity, scale, g_XMZero, rotate, translate);

		//Unproject the points on the near and far plane, with respect to the matrix we just created.
		XMVECTOR nearPoint = XMVector3Unproject(nearSource, 0.0f, 0.0f, m_ScreenDimensions.right, m_ScreenDimensions.bottom, m_deviceResources->GetScreenViewport().MinDepth, m_deviceResources->GetScreenViewport().MaxDepth, m_projection, m_view, local);

		XMVECTOR farPoint = XMVector3Unproject(farSource, 0.0f, 0.0f, m_ScreenDimensions.right, m_ScreenDimensions.bottom, m_deviceResources->GetScreenViewport().MinDepth, m_deviceResources->GetScreenViewport().MaxDepth, m_projection, m_view, local);

		//turn the transformed points into our picking vector. 
		XMVECTOR pickingVector = farPoint - nearPoint;
		pickingVector = XMVector3Normalize(pickingVector);

		//loop through mesh list for object
		for (int y = 0; y < m_displayList[i].m_model.get()->meshes.size(); y++)
		{
			
			if (m_displayList[i].m_model.get()->meshes[y]->boundingBox.Intersects(nearPoint, pickingVector, pickedDistance))
			{
				if (pickedDistance < closestDistance)
				{
					selectedID = m_displayList[i].m_ID;
					closestDistance = pickedDistance;
				}
			}
		}
	}

	//if we got a hit.  return it.  
	return selectedID;

}

void Game::CopyObject(std::vector<SceneObject>* SceneGraph, std::vector<int> selectionIDs)
{
	//setup near and far planes of frustum with mouse X and mouse y passed down from Toolmain. 
	//they may look the same but note, the difference in Z
	const XMVECTOR nearSource = XMVectorSet(m_InputCommands.mousePosX, m_InputCommands.mousePosY, 0.0f, 1.0f);
	const XMVECTOR farSource = XMVectorSet(m_InputCommands.mousePosX, m_InputCommands.mousePosY, 1.0f, 1.0f);

	//Loop through entire display list to find selected objects
	std::vector<int> copyNums;

	for (int i = 0; i < selectionIDs.size(); i++)
	{
		for (int j = 0; j < m_displayList.size(); j++)
		{
			if (m_displayList[j].m_ID == selectionIDs[i])
			{
				copyNums.push_back(j);

				break;
			}
		}
	}

	//Unproject the points on the near and far plane, with respect to the matrix we just created.
	XMVECTOR nearPoint = XMVector3Unproject(nearSource, 0.0f, 0.0f, m_ScreenDimensions.right, m_ScreenDimensions.bottom, m_deviceResources->GetScreenViewport().MinDepth, m_deviceResources->GetScreenViewport().MaxDepth, m_projection, m_view, m_world);

	XMVECTOR farPoint = XMVector3Unproject(farSource, 0.0f, 0.0f, m_ScreenDimensions.right, m_ScreenDimensions.bottom, m_deviceResources->GetScreenViewport().MinDepth, m_deviceResources->GetScreenViewport().MaxDepth, m_projection, m_view, m_world);

	//turn the transformed points into our picking vector. 
	XMVECTOR pickingVector = farPoint - nearPoint;
	pickingVector = XMVector3Normalize(pickingVector);

	//intersection with terrain
	XMVECTOR intersectPoint = XMVectorSet(0, 0, 0, 1);
	bool intersect = m_displayChunk.GetIntersection(nearPoint, pickingVector, intersectPoint);
	XMFLOAT4 intersectFloat;
	XMStoreFloat4(&intersectFloat, intersectPoint);
	
	//If there was intersection, create object copies at that point
	if (intersect)
	{
		AddObject(SceneGraph, selectionIDs, copyNums, intersectFloat.x, intersectFloat.y, intersectFloat.z);
	}

}

void Game::AddObject(std::vector<SceneObject>* SceneGraph, std::vector<int> selectionIDs, std::vector<int> copyNums, float spawnX, float spawnY, float spawnZ)
{
	//Obtain position of first object in the vector of selections
	XMFLOAT3 firstObjPos = XMFLOAT3(0, 0, 0);
	for (int j = 0; j < m_displayList.size(); j++)
	{
		if (m_displayList[j].m_ID == selectionIDs[0])
		{
			firstObjPos = XMFLOAT3(m_displayList[j].m_position.x, m_displayList[j].m_position.y, m_displayList[j].m_position.z);

			break;
		}
	}

	//First selected object will be copied at the clicked position, other copies will be offset from it in the same way as the originals
	std::vector<XMFLOAT3> objOffsets;
	//The first selected object has no offset from its own position
	objOffsets.push_back(XMFLOAT3(0, 0, 0));
	//For each selected object after the first
	for (int i = 1; i < selectionIDs.size(); i++)
	{
		for (int j = 0; j < m_displayList.size(); j++)
		{
			if (m_displayList[j].m_ID == selectionIDs[i])
			{
				//Obtain position relative to the first copied object
				float objOffsetX = m_displayList[j].m_position.x - firstObjPos.x;
				float objOffsetY = m_displayList[j].m_position.y - firstObjPos.y;
				float objOffsetZ = m_displayList[j].m_position.z - firstObjPos.z;

				//Add an offset for the corresponding copy
				objOffsets.push_back(XMFLOAT3(objOffsetX, objOffsetY, objOffsetZ));

				break;
			}
		}
	}

	//For each selected object
	for (int i = 0; i < selectionIDs.size(); i++)
	{
		//create a temp display object that we will populate then append to the display list.
		DisplayObject newDisplayObject;

		auto context = m_deviceResources->GetD3DDeviceContext();
		auto device = m_deviceResources->GetD3DDevice();

		//load model
		newDisplayObject.m_model_path = m_displayList[copyNums[i]].m_model_path;
		std::wstring modelwstr = StringToWCHART(m_displayList[copyNums[i]].m_model_path);					//convect string to Wchar

		newDisplayObject.m_model = Model::CreateFromCMO(device, modelwstr.c_str(), *m_fxFactory, true);	//get DXSDK to load model "False" for LH coordinate system (maya)

		//Load Texture
		newDisplayObject.m_tex_diffuse_path = m_displayList[copyNums[i]].m_tex_diffuse_path;
		std::wstring texturewstr = StringToWCHART(m_displayList[copyNums[i]].m_tex_diffuse_path);								//convect string to Wchar
		HRESULT rs;
		rs = CreateDDSTextureFromFile(device, texturewstr.c_str(), nullptr, &newDisplayObject.m_texture_diffuse);	//load tex into Shader resource

		//if texture fails.  load error default
		if (rs)
		{
			CreateDDSTextureFromFile(device, L"database/data/Error.dds", nullptr, &newDisplayObject.m_texture_diffuse);	//load tex into Shader resource
		}

		//apply new texture to models effect
		newDisplayObject.m_model->UpdateEffects([&](IEffect* effect) //This uses a Lambda function,  if you dont understand it: Look it up.
			{
				auto lights = dynamic_cast<BasicEffect*>(effect);
				if (lights)
				{
					lights->SetTexture(newDisplayObject.m_texture_diffuse);
				}
			});

		int newID = 1;
		bool validID = false;
		//Loop until a valid ID is found
		while (!validID)
		{
			//Find the lowest possible number that doesn't already appaear as an object ID
			//Loop through display list
			for (int i = 0; i < m_displayList.size(); i++)
			{
				//If newID is ID of an object on the list, it isn't valid
				if (m_displayList[i].m_ID == newID)
				{
					validID = false;
					break;
				}
				//If the newID isn't already in use, it is valid
				else
				{
					validID = true;
				}
			}

			//Increment newID if current one isn't valid
			if (!validID)
			{
				newID++;
			}
		}

		//set ID
		newDisplayObject.m_ID = newID;

		//set position
		newDisplayObject.m_position.x = spawnX + objOffsets[i].x;
		newDisplayObject.m_position.y = spawnY + objOffsets[i].y;
		newDisplayObject.m_position.z = spawnZ + objOffsets[i].z;

		//set orientation
		newDisplayObject.m_orientation.x = m_displayList[copyNums[i]].m_orientation.x;
		newDisplayObject.m_orientation.y = m_displayList[copyNums[i]].m_orientation.y;
		newDisplayObject.m_orientation.z = m_displayList[copyNums[i]].m_orientation.z;

		//set scale
		newDisplayObject.m_scale.x = m_displayList[copyNums[i]].m_scale.x;
		newDisplayObject.m_scale.y = m_displayList[copyNums[i]].m_scale.y;
		newDisplayObject.m_scale.z = m_displayList[copyNums[i]].m_scale.z;

		//set wireframe / render flags
		newDisplayObject.m_render = m_displayList[copyNums[i]].m_render;
		newDisplayObject.m_wireframe = m_displayList[copyNums[i]].m_wireframe;

		//set lighting values
		newDisplayObject.m_light_type = m_displayList[copyNums[i]].m_light_type;
		newDisplayObject.m_light_diffuse_r = m_displayList[copyNums[i]].m_light_diffuse_r;
		newDisplayObject.m_light_diffuse_g = m_displayList[copyNums[i]].m_light_diffuse_g;
		newDisplayObject.m_light_diffuse_b = m_displayList[copyNums[i]].m_light_diffuse_b;
		newDisplayObject.m_light_specular_r = m_displayList[copyNums[i]].m_light_specular_r;
		newDisplayObject.m_light_specular_g = m_displayList[copyNums[i]].m_light_specular_g;
		newDisplayObject.m_light_specular_b = m_displayList[copyNums[i]].m_light_specular_b;
		newDisplayObject.m_light_spot_cutoff = m_displayList[copyNums[i]].m_light_spot_cutoff;
		newDisplayObject.m_light_constant = m_displayList[copyNums[i]].m_light_constant;
		newDisplayObject.m_light_linear = m_displayList[copyNums[i]].m_light_linear;
		newDisplayObject.m_light_quadratic = m_displayList[copyNums[i]].m_light_quadratic;

		//Add new object to display list
		m_displayList.push_back(newDisplayObject);
	}
}

void Game::DragObject(std::vector<SceneObject>* SceneGraph, std::vector<int> selectionIDs)
{
	//setup near and far planes of frustum with mouse X and mouse y passed down from Toolmain. 
	//they may look the same but note, the difference in Z
	const XMVECTOR nearSource = XMVectorSet(m_InputCommands.mousePosX, m_InputCommands.mousePosY, 0.0f, 1.0f);
	const XMVECTOR farSource = XMVectorSet(m_InputCommands.mousePosX, m_InputCommands.mousePosY, 1.0f, 1.0f);

	//Unproject the points on the near and far plane, with respect to the matrix we just created.
	XMVECTOR nearPoint = XMVector3Unproject(nearSource, 0.0f, 0.0f, m_ScreenDimensions.right, m_ScreenDimensions.bottom, m_deviceResources->GetScreenViewport().MinDepth, m_deviceResources->GetScreenViewport().MaxDepth, m_projection, m_view, m_world);

	XMVECTOR farPoint = XMVector3Unproject(farSource, 0.0f, 0.0f, m_ScreenDimensions.right, m_ScreenDimensions.bottom, m_deviceResources->GetScreenViewport().MinDepth, m_deviceResources->GetScreenViewport().MaxDepth, m_projection, m_view, m_world);

	//turn the transformed points into our picking vector. 
	XMVECTOR pickingVector = farPoint - nearPoint;
	pickingVector = XMVector3Normalize(pickingVector);

	//intersection with terrain
	XMVECTOR intersectPoint = XMVectorSet(0, 0, 0, 1);
	bool intersect = m_displayChunk.GetIntersection(nearPoint, pickingVector, intersectPoint);
	XMFLOAT4 intersectFloat;
	XMStoreFloat4(&intersectFloat, intersectPoint);

	//If there is intersection
	if (intersect)
	{
		//Obtain the position of the first object in the vector of selections
		XMFLOAT3 firstObjPos = XMFLOAT3(0, 0, 0);
		for (int j = 0; j < m_displayList.size(); j++)
		{
			if (m_displayList[j].m_ID == selectionIDs[0])
			{
				firstObjPos = XMFLOAT3(m_displayList[j].m_position.x, m_displayList[j].m_position.y, m_displayList[j].m_position.z);

				break;
			}
		}

		//The first selected object will be dragged to the mouse selection, others will be dragged relative to their inital offset from it
		//For each selected object
		for (int i = 0; i < selectionIDs.size(); i++)
		{
			for (int j = 0; j < m_displayList.size(); j++)
			{
				if (m_displayList[j].m_ID == selectionIDs[i])
				{
					//Obtain offset by subtracting first object's intital position from current object's intial position
					//First object will have offset of (0, 0, 0) from its own position
					float objOffsetX = m_displayList[j].m_position.x - firstObjPos.x;
					float objOffsetY = m_displayList[j].m_position.y - firstObjPos.y;
					float objOffsetZ = m_displayList[j].m_position.z - firstObjPos.z;
					
					//New position is mouse intersect plus offset
					m_displayList[j].m_position.x = intersectFloat.x + objOffsetX;
					m_displayList[j].m_position.y = intersectFloat.y + objOffsetY;
					m_displayList[j].m_position.z = intersectFloat.z + objOffsetZ;

					break;
				}
			}
		}
	}
}

void Game::RotateObject(bool rotX, bool rotY, bool rotZ, int rotMod, float speedMod, std::vector<int> selectionIDs)
{
	//Search display list for selected objects
	for (int i = 0; i < selectionIDs.size(); i++)
	{
		for (int j = 0; j < m_displayList.size(); j++)
		{
			if (m_displayList[j].m_ID == selectionIDs[i])
			{
				//Apply appropriate rotation
				if (rotX)
				{
					m_displayList[j].m_orientation.x += speedMod * rotMod;
				}
				if (rotY)
				{
					m_displayList[j].m_orientation.y += speedMod * rotMod;
				}
				if (rotZ)
				{
					m_displayList[j].m_orientation.z += speedMod * rotMod;
				}
			}
		}
	}
}

void Game::ObjectFog(bool on, std::vector<int> objectIDs)
{
	//Search display list for selected objects
	for (int i = 0; i < objectIDs.size(); i++)
	{
		for (int j = 0; j < m_displayList.size(); j++)
		{
			if (m_displayList[j].m_ID == objectIDs[i])
			{
				//Enable or disable fog effect
				m_displayList[j].m_model->UpdateEffects([&](IEffect* effect)
					{
						auto fog = dynamic_cast<IEffectFog*>(effect);
						if (fog)
						{
							fog->SetFogEnabled(on);
							fog->SetFogStart(6); // assuming RH coordiantes
							fog->SetFogEnd(6);
							fog->SetFogColor(Colors::AliceBlue);
						}
					});
				
			}
		}
	}
}

void Game::RemoveObjects(std::vector<int> objectIDs)
{
	//Search display list for selected objects
	for (int i = 0; i < objectIDs.size(); i++)
	{
		for (int j = 0; j < m_displayList.size(); j++)
		{
			if (m_displayList[j].m_ID == objectIDs[i])
			{
				//Erase those objects
				m_displayList.erase(m_displayList.begin() + j);
			}
		}
	}
}

std::vector<DisplayObject> Game::GetDisplayList()
{
	return m_displayList;
}

void Game::ScaleObject(bool scaleX, bool scaleY, bool scaleZ, int scaleMod, float speedMod, std::vector<int> selectionIDs)
{
	//Search display list for selected objects
	for (int i = 0; i < selectionIDs.size(); i++)
	{
		for (int j = 0; j < m_displayList.size(); j++)
		{
			if (m_displayList[j].m_ID == selectionIDs[i])
			{
				//Apply appropriate rotation
				if (scaleX)
				{
					m_displayList[j].m_scale.x += speedMod * scaleMod;
				}
				if (scaleY)
				{
					m_displayList[j].m_scale.y += speedMod * scaleMod;
				}
				if (scaleZ)
				{
					m_displayList[j].m_scale.z += speedMod * scaleMod;
				}
			}
		}
	}
}