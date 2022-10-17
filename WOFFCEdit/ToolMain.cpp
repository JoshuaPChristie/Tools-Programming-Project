#include "ToolMain.h"
#include "resource.h"
#include <vector>
#include <sstream>

//
//ToolMain Class
ToolMain::ToolMain()
{

	m_currentChunk = 0;		//default value
	m_sceneGraph.clear();	//clear the vector for the scenegraph
	m_databaseConnection = NULL;

	//zero input commands
	m_toolInputCommands.forward		= false;
	m_toolInputCommands.back		= false;
	m_toolInputCommands.left		= false;
	m_toolInputCommands.right		= false;
	
}


ToolMain::~ToolMain()
{
	sqlite3_close(m_databaseConnection);		//close the database connection
}

std::vector<int> ToolMain::getCurrentSelectionID()
{
	return m_selectedObjects;
}

void ToolMain::onActionInitialise(HWND handle, int width, int height)
{
	//window size, handle etc for directX
	m_width		= width;
	m_height	= height;
	
	m_d3dRenderer.Initialize(handle, m_width, m_height);

	//database connection establish
	int rc;
	rc = sqlite3_open_v2("database/test.db",&m_databaseConnection, SQLITE_OPEN_READWRITE, NULL);

	if (rc) 
	{
		TRACE("Can't open database");
		//if the database cant open. Perhaps a more catastrophic error would be better here
	}
	else 
	{
		TRACE("Opened database successfully");
	}

	onActionLoad();
}

void ToolMain::onActionLoad()
{
	//load current chunk and objects into lists
	if (!m_sceneGraph.empty())		//is the vector empty
	{
		m_sceneGraph.clear();		//if not, empty it
	}

	//SQL
	int rc;
	char *sqlCommand;
	char *ErrMSG = 0;
	sqlite3_stmt *pResults;								//results of the query
	sqlite3_stmt *pResultsChunk;

	//OBJECTS IN THE WORLD
	//prepare SQL Text
	sqlCommand = "SELECT * from Objects";				//sql command which will return all records from the objects table.
	//Send Command and fill result object
	rc = sqlite3_prepare_v2(m_databaseConnection, sqlCommand, -1, &pResults, 0 );
	
	//loop for each row in results until there are no more rows.  ie for every row in the results. We create and object
	while (sqlite3_step(pResults) == SQLITE_ROW)
	{	
		SceneObject newSceneObject;
		newSceneObject.ID = sqlite3_column_int(pResults, 0);
		newSceneObject.chunk_ID = sqlite3_column_int(pResults, 1);
		newSceneObject.model_path		= reinterpret_cast<const char*>(sqlite3_column_text(pResults, 2));
		newSceneObject.tex_diffuse_path = reinterpret_cast<const char*>(sqlite3_column_text(pResults, 3));
		newSceneObject.posX = sqlite3_column_double(pResults, 4);
		newSceneObject.posY = sqlite3_column_double(pResults, 5);
		newSceneObject.posZ = sqlite3_column_double(pResults, 6);
		newSceneObject.rotX = sqlite3_column_double(pResults, 7);
		newSceneObject.rotY = sqlite3_column_double(pResults, 8);
		newSceneObject.rotZ = sqlite3_column_double(pResults, 9);
		newSceneObject.scaX = sqlite3_column_double(pResults, 10);
		newSceneObject.scaY = sqlite3_column_double(pResults, 11);
		newSceneObject.scaZ = sqlite3_column_double(pResults, 12);
		newSceneObject.render = sqlite3_column_int(pResults, 13);
		newSceneObject.collision = sqlite3_column_int(pResults, 14);
		newSceneObject.collision_mesh = reinterpret_cast<const char*>(sqlite3_column_text(pResults, 15));
		newSceneObject.collectable = sqlite3_column_int(pResults, 16);
		newSceneObject.destructable = sqlite3_column_int(pResults, 17);
		newSceneObject.health_amount = sqlite3_column_int(pResults, 18);
		newSceneObject.editor_render = sqlite3_column_int(pResults, 19);
		newSceneObject.editor_texture_vis = sqlite3_column_int(pResults, 20);
		newSceneObject.editor_normals_vis = sqlite3_column_int(pResults, 21);
		newSceneObject.editor_collision_vis = sqlite3_column_int(pResults, 22);
		newSceneObject.editor_pivot_vis = sqlite3_column_int(pResults, 23);
		newSceneObject.pivotX = sqlite3_column_double(pResults, 24);
		newSceneObject.pivotY = sqlite3_column_double(pResults, 25);
		newSceneObject.pivotZ = sqlite3_column_double(pResults, 26);
		newSceneObject.snapToGround = sqlite3_column_int(pResults, 27);
		newSceneObject.AINode = sqlite3_column_int(pResults, 28);
		newSceneObject.audio_path = reinterpret_cast<const char*>(sqlite3_column_text(pResults, 29));
		newSceneObject.volume = sqlite3_column_double(pResults, 30);
		newSceneObject.pitch = sqlite3_column_double(pResults, 31);
		newSceneObject.pan = sqlite3_column_int(pResults, 32);
		newSceneObject.one_shot = sqlite3_column_int(pResults, 33);
		newSceneObject.play_on_init = sqlite3_column_int(pResults, 34);
		newSceneObject.play_in_editor = sqlite3_column_int(pResults, 35);
		newSceneObject.min_dist = sqlite3_column_double(pResults, 36);
		newSceneObject.max_dist = sqlite3_column_double(pResults, 37);
		newSceneObject.camera = sqlite3_column_int(pResults, 38);
		newSceneObject.path_node = sqlite3_column_int(pResults, 39);
		newSceneObject.path_node_start = sqlite3_column_int(pResults, 40);
		newSceneObject.path_node_end = sqlite3_column_int(pResults, 41);
		newSceneObject.parent_id = sqlite3_column_int(pResults, 42);
		newSceneObject.editor_wireframe = sqlite3_column_int(pResults, 43);
		newSceneObject.name = reinterpret_cast<const char*>(sqlite3_column_text(pResults, 44));

		newSceneObject.light_type = sqlite3_column_int(pResults, 45);
		newSceneObject.light_diffuse_r = sqlite3_column_double(pResults, 46);
		newSceneObject.light_diffuse_g = sqlite3_column_double(pResults, 47);
		newSceneObject.light_diffuse_b = sqlite3_column_double(pResults, 48);
		newSceneObject.light_specular_r = sqlite3_column_double(pResults, 49);
		newSceneObject.light_specular_g = sqlite3_column_double(pResults, 50);
		newSceneObject.light_specular_b = sqlite3_column_double(pResults, 51);
		newSceneObject.light_spot_cutoff = sqlite3_column_double(pResults, 52);
		newSceneObject.light_constant = sqlite3_column_double(pResults, 53);
		newSceneObject.light_linear = sqlite3_column_double(pResults, 54);
		newSceneObject.light_quadratic = sqlite3_column_double(pResults, 55);
	

		//send completed object to scenegraph
		m_sceneGraph.push_back(newSceneObject);
	}

	//THE WORLD CHUNK
	//prepare SQL Text
	sqlCommand = "SELECT * from Chunks";				//sql command which will return all records from  chunks table. There is only one tho.
														//Send Command and fill result object
	rc = sqlite3_prepare_v2(m_databaseConnection, sqlCommand, -1, &pResultsChunk, 0);


	sqlite3_step(pResultsChunk);
	m_chunk.ID = sqlite3_column_int(pResultsChunk, 0);
	m_chunk.name = reinterpret_cast<const char*>(sqlite3_column_text(pResultsChunk, 1));
	m_chunk.chunk_x_size_metres = sqlite3_column_int(pResultsChunk, 2);
	m_chunk.chunk_y_size_metres = sqlite3_column_int(pResultsChunk, 3);
	m_chunk.chunk_base_resolution = sqlite3_column_int(pResultsChunk, 4);
	m_chunk.heightmap_path = reinterpret_cast<const char*>(sqlite3_column_text(pResultsChunk, 5));
	m_chunk.tex_diffuse_path = reinterpret_cast<const char*>(sqlite3_column_text(pResultsChunk, 6));
	m_chunk.tex_splat_alpha_path = reinterpret_cast<const char*>(sqlite3_column_text(pResultsChunk, 7));
	m_chunk.tex_splat_1_path = reinterpret_cast<const char*>(sqlite3_column_text(pResultsChunk, 8));
	m_chunk.tex_splat_2_path = reinterpret_cast<const char*>(sqlite3_column_text(pResultsChunk, 9));
	m_chunk.tex_splat_3_path = reinterpret_cast<const char*>(sqlite3_column_text(pResultsChunk, 10));
	m_chunk.tex_splat_4_path = reinterpret_cast<const char*>(sqlite3_column_text(pResultsChunk, 11));
	m_chunk.render_wireframe = sqlite3_column_int(pResultsChunk, 12);
	m_chunk.render_normals = sqlite3_column_int(pResultsChunk, 13);
	m_chunk.tex_diffuse_tiling = sqlite3_column_int(pResultsChunk, 14);
	m_chunk.tex_splat_1_tiling = sqlite3_column_int(pResultsChunk, 15);
	m_chunk.tex_splat_2_tiling = sqlite3_column_int(pResultsChunk, 16);
	m_chunk.tex_splat_3_tiling = sqlite3_column_int(pResultsChunk, 17);
	m_chunk.tex_splat_4_tiling = sqlite3_column_int(pResultsChunk, 18);


	//Process REsults into renderable
	m_d3dRenderer.BuildDisplayList(&m_sceneGraph);
	//build the renderable chunk 
	m_d3dRenderer.BuildDisplayChunk(&m_chunk);

}

void ToolMain::onActionSave()
{
	//Load changes from display list
	loadSceneGraph(&m_d3dRenderer.GetDisplayList());
	
	//SQL
	int rc;
	char *sqlCommand;
	char *ErrMSG = 0;
	sqlite3_stmt *pResults;								//results of the query
	

	//OBJECTS IN THE WORLD Delete them all
	//prepare SQL Text
	sqlCommand = "DELETE FROM Objects";	 //will delete the whole object table.   Slightly risky but hey.
	rc = sqlite3_prepare_v2(m_databaseConnection, sqlCommand, -1, &pResults, 0);
	sqlite3_step(pResults);

	//Populate with our new objects
	std::wstring sqlCommand2;
	int numObjects = m_sceneGraph.size();	//Loop thru the scengraph.

	for (int i = 0; i < numObjects; i++)
	{
		std::stringstream command;
		command << "INSERT INTO Objects " 
			<<"VALUES(" << m_sceneGraph.at(i).ID << ","
			<< m_sceneGraph.at(i).chunk_ID  << ","
			<< "'" << m_sceneGraph.at(i).model_path <<"'" << ","
			<< "'" << m_sceneGraph.at(i).tex_diffuse_path << "'" << ","
			<< m_sceneGraph.at(i).posX << ","
			<< m_sceneGraph.at(i).posY << ","
			<< m_sceneGraph.at(i).posZ << ","
			<< m_sceneGraph.at(i).rotX << ","
			<< m_sceneGraph.at(i).rotY << ","
			<< m_sceneGraph.at(i).rotZ << ","
			<< m_sceneGraph.at(i).scaX << ","
			<< m_sceneGraph.at(i).scaY << ","
			<< m_sceneGraph.at(i).scaZ << ","
			<< m_sceneGraph.at(i).render << ","
			<< m_sceneGraph.at(i).collision << ","
			<< "'" << m_sceneGraph.at(i).collision_mesh << "'" << ","
			<< m_sceneGraph.at(i).collectable << ","
			<< m_sceneGraph.at(i).destructable << ","
			<< m_sceneGraph.at(i).health_amount << ","
			<< m_sceneGraph.at(i).editor_render << ","
			<< m_sceneGraph.at(i).editor_texture_vis << ","
			<< m_sceneGraph.at(i).editor_normals_vis << ","
			<< m_sceneGraph.at(i).editor_collision_vis << ","
			<< m_sceneGraph.at(i).editor_pivot_vis << ","
			<< m_sceneGraph.at(i).pivotX << ","
			<< m_sceneGraph.at(i).pivotY << ","
			<< m_sceneGraph.at(i).pivotZ << ","
			<< m_sceneGraph.at(i).snapToGround << ","
			<< m_sceneGraph.at(i).AINode << ","
			<< "'" << m_sceneGraph.at(i).audio_path << "'" << ","
			<< m_sceneGraph.at(i).volume << ","
			<< m_sceneGraph.at(i).pitch << ","
			<< m_sceneGraph.at(i).pan << ","
			<< m_sceneGraph.at(i).one_shot << ","
			<< m_sceneGraph.at(i).play_on_init << ","
			<< m_sceneGraph.at(i).play_in_editor << ","
			<< m_sceneGraph.at(i).min_dist << ","
			<< m_sceneGraph.at(i).max_dist << ","
			<< m_sceneGraph.at(i).camera << ","
			<< m_sceneGraph.at(i).path_node << ","
			<< m_sceneGraph.at(i).path_node_start << ","
			<< m_sceneGraph.at(i).path_node_end << ","
			<< m_sceneGraph.at(i).parent_id << ","
			<< m_sceneGraph.at(i).editor_wireframe << ","
			<< "'" << m_sceneGraph.at(i).name << "'" << ","

			<< m_sceneGraph.at(i).light_type << ","
			<< m_sceneGraph.at(i).light_diffuse_r << ","
			<< m_sceneGraph.at(i).light_diffuse_g << ","
			<< m_sceneGraph.at(i).light_diffuse_b << ","
			<< m_sceneGraph.at(i).light_specular_r << ","
			<< m_sceneGraph.at(i).light_specular_g << ","
			<< m_sceneGraph.at(i).light_specular_b << ","
			<< m_sceneGraph.at(i).light_spot_cutoff << ","
			<< m_sceneGraph.at(i).light_constant << ","
			<< m_sceneGraph.at(i).light_linear << ","
			<< m_sceneGraph.at(i).light_quadratic

			<< ")";
		std::string sqlCommand2 = command.str();
		rc = sqlite3_prepare_v2(m_databaseConnection, sqlCommand2.c_str(), -1, &pResults, 0);
		sqlite3_step(pResults);	
	}
	MessageBox(NULL, L"Objects Saved", L"Notification", MB_OK);
}

void ToolMain::onActionSaveTerrain()
{
	m_d3dRenderer.SaveDisplayChunk(&m_chunk);
}

void ToolMain::Tick(MSG *msg)
{
	//do we have a selection
	//do we have a mode
	//are we clicking / dragging /releasing
	//has something changed
		//update Scenegraph
		//add to scenegraph
		//resend scenegraph to Direct X renderer

	//Renderer Update Call
	m_d3dRenderer.Tick(&m_toolInputCommands);
	//If the right mouse button isn't down
	if (!m_toolInputCommands.cameraDrag)
	{
		//If the left mouse button is down
		if (m_toolInputCommands.mouseLDown)
		{
			//If the R, T or Y keys are down
			if (m_toolInputCommands.rotDragX || m_toolInputCommands.rotDragY || m_toolInputCommands.rotDragZ)
			{
				//Rotate x axis with R
				if (m_toolInputCommands.rotDragX)
				{
					//Positive rotation
					if (m_toolInputCommands.rotObjXPos)
					{
						m_d3dRenderer.RotateObject(true, false, false, 1, m_toolInputCommands.mouseSpeedY, m_selectedObjects);
					}
					//Negative rotation
					else if (m_toolInputCommands.rotObjXNeg)
					{
						m_d3dRenderer.RotateObject(true, false, false, -1, m_toolInputCommands.mouseSpeedY, m_selectedObjects);
					}
				}
				//Rotate y axis with T
				if (m_toolInputCommands.rotDragY)
				{
					//Positive rotation
					if (m_toolInputCommands.rotObjYPos)
					{
						m_d3dRenderer.RotateObject(false, true, false, 1, m_toolInputCommands.mouseSpeedY, m_selectedObjects);
					}
					//Negative rotation
					else if (m_toolInputCommands.rotObjYNeg)
					{
						m_d3dRenderer.RotateObject(false, true, false, -1, m_toolInputCommands.mouseSpeedY, m_selectedObjects);
					}
				}
				//Rotate z axis with Y
				if (m_toolInputCommands.rotDragZ)
				{
					//Positive rotation
					if (m_toolInputCommands.rotObjZPos)
					{
						m_d3dRenderer.RotateObject(false, false, true, 1, m_toolInputCommands.mouseSpeedY, m_selectedObjects);
					}
					//Negative rotation
					else if (m_toolInputCommands.rotObjZNeg)
					{
						m_d3dRenderer.RotateObject(false, false, true, -1, m_toolInputCommands.mouseSpeedY, m_selectedObjects);
					}
				}
			}
			//Otherwise, if the F, G or H keys are down
			else if (m_toolInputCommands.scaleDragX || m_toolInputCommands.scaleDragY || m_toolInputCommands.scaleDragZ)
			{
				//X scaling with F
				if (m_toolInputCommands.scaleDragX)
				{
					//Scale up
					if (m_toolInputCommands.scaleObjXUp)
					{
						m_d3dRenderer.ScaleObject(true, false, false, 1, m_toolInputCommands.mouseSpeedY, m_selectedObjects);
					}
					//Scale down
					else if (m_toolInputCommands.scaleObjXDown)
					{
						m_d3dRenderer.ScaleObject(true, false, false, -1, m_toolInputCommands.mouseSpeedY, m_selectedObjects);
					}
				}
				//Y scaling with G
				if (m_toolInputCommands.scaleDragY)
				{
					//Scale up
					if (m_toolInputCommands.scaleObjYUp)
					{
						m_d3dRenderer.ScaleObject(false, true, false, 1, m_toolInputCommands.mouseSpeedY, m_selectedObjects);
					}
					//Scale down
					else if (m_toolInputCommands.scaleObjYDown)
					{
						m_d3dRenderer.ScaleObject(false, true, false, -1, m_toolInputCommands.mouseSpeedY, m_selectedObjects);
					}
				}
				//Z scaling with H
				if (m_toolInputCommands.scaleDragZ)
				{
					//Scale up
					if (m_toolInputCommands.scaleObjZUp)
					{
						m_d3dRenderer.ScaleObject(false, false, true, 1, m_toolInputCommands.mouseSpeedY, m_selectedObjects);
					}
					//Scale down
					else if (m_toolInputCommands.scaleObjZDown)
					{
						m_d3dRenderer.ScaleObject(false, false, true, -1, m_toolInputCommands.mouseSpeedY, m_selectedObjects);
					}
				}
			}
			//Otherwise if C key is down
			else if (m_toolInputCommands.copySelection)
			{
				//When button is first clicked
				if (m_toolInputCommands.mouseLClick)
				{
					//Copy currently selected objects
					if (!m_selectedObjects.empty())
					{
						m_d3dRenderer.CopyObject(&m_sceneGraph, m_selectedObjects);
					}
				}
				//Initial click has ended
				m_toolInputCommands.mouseLClick = false;
			}
			//Otherwise, if the button is first being clicked
			else if (m_toolInputCommands.mouseLClick)
			{
				//Get object that was clicked on
				int mouseSelection = m_d3dRenderer.MousePicking();

				bool addToList = true;
				//Check if the clicked object is already selected
				for (int i = 0; i < m_selectedObjects.size(); i++)
				{
					//If so
					if (m_selectedObjects[i] == mouseSelection)
					{
						//Object will not be added to vector as new item
						addToList = false;
						//Move existing item to the front of the vector
						std::vector<int> objectToRemove;
						objectToRemove.push_back(m_selectedObjects[i]);
						m_d3dRenderer.ObjectFog(true, objectToRemove);
						m_selectedObjects.erase(m_selectedObjects.begin() + i);
						m_selectedObjects.insert(m_selectedObjects.begin(), mouseSelection);
					}
				}

				//If the user is not making multiple selections, de-select all objects
				if (m_toolInputCommands.multiPick == false)
				{
					m_d3dRenderer.ObjectFog(false, m_selectedObjects);
					m_selectedObjects.clear();
				}

				//If object was not already selected
				if (addToList)
				{
					//Select it
					if (mouseSelection != -1)
					{
						m_selectedObjects.insert(m_selectedObjects.begin(), mouseSelection);
					}
				}

				//Initial click has ended
				m_toolInputCommands.mouseLClick = false;
			}
			//Otherwise, while buttton is held
			else
			{
				//If objects are selected
				if (!m_selectedObjects.empty())
				{
					//Drag them across terrain
					m_d3dRenderer.DragObject(&m_sceneGraph, m_selectedObjects);
				}
			}
		}
	}
	//Otherwise
	else
	{
		//de - select all objects
		m_d3dRenderer.ObjectFog(false, m_selectedObjects);
		m_selectedObjects.clear();
	}

	//If objects are selected
	if (!m_selectedObjects.empty())
	{
		//Highlight them
		m_d3dRenderer.ObjectFog(true, m_selectedObjects);
	}

	//If delete button up
	if (!m_toolInputCommands.deleteButton)
	{
		//Button ready to be pressed again
		m_toolInputCommands.deleteButtonPressed = true;
	}
	//If delete button down
	else
	{
		//If button is first being pressed
		if (m_toolInputCommands.deleteButtonPressed)
		{
			//If objects are selected
			if (!m_selectedObjects.empty())
			{
				//Delete them
				m_d3dRenderer.RemoveObjects(m_selectedObjects);
			}
			//Initial press has ended
			m_toolInputCommands.deleteButtonPressed = false;
		}
	}

	//If save button up
	if (!m_toolInputCommands.saveButton)
	{
		//Button ready to be pressed again
		m_toolInputCommands.saveButtonPressed = true;
	}
	//If save button up
	else
	{
		//If button is first being pressed
		if (m_toolInputCommands.saveButtonPressed)
		{
			//Save objects
			onActionSave();
			//Initial press has ended
			m_toolInputCommands.saveButtonPressed = false;
		}
	}


}

void ToolMain::UpdateInput(MSG * msg)
{

	switch (msg->message)
	{
		//Global inputs,  mouse position and keys etc
	case WM_KEYDOWN:
		m_keyArray[msg->wParam] = true;
		break;

	case WM_KEYUP:
		m_keyArray[msg->wParam] = false;
		break;

	case WM_MOUSEMOVE:

		//Update mouse position whenever mouse moves
		m_toolInputCommands.mousePosX = GET_X_LPARAM(msg->lParam);
		m_toolInputCommands.mousePosY = GET_Y_LPARAM(msg->lParam);

		//if right mouse button down
		if (m_toolInputCommands.cameraDrag)
		{

			float mouseDragX;
			float mouseDragY;
			//Calculate how far mouse has been dragged since initial click
			mouseDragX = m_toolInputCommands.mousePosX - m_toolInputCommands.mousePosPrevX;
			mouseDragY = m_toolInputCommands.mousePosY - m_toolInputCommands.mousePosPrevY;

			//Horizontal drag
			//If negative, rotate camera left
			if (mouseDragX < 0)
			{
				m_toolInputCommands.rotRight = false;
				m_toolInputCommands.rotLeft = true;
			}
			//If positive, rotate right
			if (mouseDragX > 0)
			{
				m_toolInputCommands.rotRight = true;
				m_toolInputCommands.rotLeft = false;
			}

			//Set horizontal rotation speed according to distance of drag
			if (abs(mouseDragX) > 100)
			{
				m_toolInputCommands.mouseSpeedX = 1;
			}
			else if (abs(mouseDragX) < 20)
			{
				m_toolInputCommands.mouseSpeedX = 0.05;
			}
			else
			{
				m_toolInputCommands.mouseSpeedX = abs(mouseDragX) * 0.01;
			}

			//Vertical drag
			//If negative, rotate up
			if (mouseDragY < 0)
			{
				m_toolInputCommands.rotUp = true;
				m_toolInputCommands.rotDown = false;
			}
			//If positive, rotate down
			if (mouseDragY > 0)
			{
				m_toolInputCommands.rotUp = false;
				m_toolInputCommands.rotDown = true;
			}

			//Set vertical rotation speed according to distance of drag
			if (abs(mouseDragY) > 100)
			{
				m_toolInputCommands.mouseSpeedY = 1;
			}
			else if (abs(mouseDragY) < 20)
			{
				m_toolInputCommands.mouseSpeedY = 0.05;
			}
			else
			{
				m_toolInputCommands.mouseSpeedY = abs(mouseDragY) * 0.01;
			}

		}

		//If R, T or Y keys down
		if (m_toolInputCommands.rotDragX || m_toolInputCommands.rotDragY || m_toolInputCommands.rotDragZ)
		{
			float mouseDragX;
			float mouseDragY;
			//Calculate how far mouse has been dragged since initial click
			mouseDragX = m_toolInputCommands.mousePosX - m_toolInputCommands.mousePosPrevX;
			mouseDragY = m_toolInputCommands.mousePosY - m_toolInputCommands.mousePosPrevY;

			//If R key down, apply x axis rotation
			if (m_toolInputCommands.rotDragX)
			{
				//If vertical drag negative, rotation positive
				if (mouseDragY < 0)
				{
					m_toolInputCommands.rotObjXPos = true;
					m_toolInputCommands.rotObjXNeg = false;
				}
				//If vertical drag positive, rotation negative
				if (mouseDragY > 0)
				{
					m_toolInputCommands.rotObjXPos = false;
					m_toolInputCommands.rotObjXNeg = true;
				}
			}
			//If T key down, apply y axis rotation
			if (m_toolInputCommands.rotDragY)
			{
				//If vertical drag negative, rotation positive
				if (mouseDragY < 0)
				{
					m_toolInputCommands.rotObjYPos = true;
					m_toolInputCommands.rotObjYNeg = false;
				}
				//If vertical drag positive, rotation negative
				if (mouseDragY > 0)
				{
					m_toolInputCommands.rotObjYPos = false;
					m_toolInputCommands.rotObjYNeg = true;
				}
			}
			//If Y key down, apply z axis rotation
			if (m_toolInputCommands.rotDragZ)
			{
				//If vertical drag negative, rotation positive
				if (mouseDragY < 0)
				{
					m_toolInputCommands.rotObjZPos = true;
					m_toolInputCommands.rotObjZNeg = false;
				}
				//If vertical drag positive, rotation negative
				if (mouseDragY > 0)
				{
					m_toolInputCommands.rotObjZPos = false;
					m_toolInputCommands.rotObjZNeg = true;
				}
			}

			//Set rotation speed according to distance of drag
			if (abs(mouseDragY) > 120)
			{
				m_toolInputCommands.mouseSpeedY = 3;
			}
			else if (abs(mouseDragY) > 80)
			{
				m_toolInputCommands.mouseSpeedY = 2;
			}
			else if (abs(mouseDragY) > 40)
			{
				m_toolInputCommands.mouseSpeedY = 1;
			}
			else if (abs(mouseDragY) < 40)
			{
				m_toolInputCommands.mouseSpeedY = 0.5;
			}
			else
			{
				m_toolInputCommands.mouseSpeedY = 0;
			}
		}

		//If F, G or H keys down
		if (m_toolInputCommands.scaleDragX || m_toolInputCommands.scaleDragY || m_toolInputCommands.scaleDragZ)
		{
			float mouseDragX;
			float mouseDragY;
			//Calculate how far mouse has been dragged since initial click
			mouseDragX = m_toolInputCommands.mousePosX - m_toolInputCommands.mousePosPrevX;
			mouseDragY = m_toolInputCommands.mousePosY - m_toolInputCommands.mousePosPrevY;

			//If F key down, apply x scaling
			if (m_toolInputCommands.scaleDragX)
			{
				//If vertical drag negative, scale up
				if (mouseDragY < 0)
				{
					m_toolInputCommands.scaleObjXUp = true;
					m_toolInputCommands.scaleObjXDown = false;
				}
				//If vertical drag positive, scale down
				if (mouseDragY > 0)
				{
					m_toolInputCommands.scaleObjXUp = false;
					m_toolInputCommands.scaleObjXDown = true;
				}
			}
			//If G key down, apply y scaling
			if (m_toolInputCommands.scaleDragY)
			{
				if (mouseDragY < 0)
				{
					//If vertical drag negative, scale up
					m_toolInputCommands.scaleObjYUp = true;
					m_toolInputCommands.scaleObjYDown = false;
				}
				if (mouseDragY > 0)
				{
					//If vertical drag positive, scale down
					m_toolInputCommands.scaleObjYUp = false;
					m_toolInputCommands.scaleObjYDown = true;
				}
			}
			//If H key down, apply z scaling
			if (m_toolInputCommands.scaleDragZ)
			{
				if (mouseDragY < 0)
				{
					//If vertical drag negative, scale up
					m_toolInputCommands.scaleObjZUp = true;
					m_toolInputCommands.scaleObjZDown = false;
				}
				if (mouseDragY > 0)
				{
					m_toolInputCommands.scaleObjZUp = false;
					m_toolInputCommands.scaleObjZDown = true;
				}
			}

			//Set scaling speed according to distance of drag
			if (abs(mouseDragY) > 120)
			{
				m_toolInputCommands.mouseSpeedY = 0.02;
			}
			else if (abs(mouseDragY) > 80)
			{
				m_toolInputCommands.mouseSpeedY = 0.01;
			}
			else if (abs(mouseDragY) > 40)
			{
				m_toolInputCommands.mouseSpeedY = 0.005;
			}
			else if (abs(mouseDragY) < 40)
			{
				m_toolInputCommands.mouseSpeedY = 0.001;
			}
			else
			{
				m_toolInputCommands.mouseSpeedY = 0;
			}
		}

		break;

	case WM_LBUTTONDOWN:
		//mouse left pressed.	
		m_toolInputCommands.mouseLDown = true;
		m_toolInputCommands.mouseLClick = true;

		//If R, T, Y, F, G or H keys down, start mouse drag
		if (m_toolInputCommands.rotDragX || m_toolInputCommands.rotDragY || m_toolInputCommands.rotDragZ || m_toolInputCommands.scaleDragX || m_toolInputCommands.scaleDragY || m_toolInputCommands.scaleDragZ)
		{
			m_toolInputCommands.mousePosPrevX = GET_X_LPARAM(msg->lParam);
			m_toolInputCommands.mousePosPrevY = GET_Y_LPARAM(msg->lParam);
		}

		break;

	case WM_LBUTTONUP:
		//If right mouse button up, set related bools false	
		m_toolInputCommands.mouseLDown = false;
		m_toolInputCommands.mouseLClick = false;
		break;


	case WM_RBUTTONDOWN:	//mouse button down,  you will probably need to check when its up too
		//set some flag for the mouse button in inputcommands
		m_toolInputCommands.cameraDrag = true;

		//Start mouse drag
		m_toolInputCommands.mousePosPrevX = GET_X_LPARAM(msg->lParam);
		m_toolInputCommands.mousePosPrevY = GET_Y_LPARAM(msg->lParam);
		
		break;

	case WM_RBUTTONUP:
		//If right mouse button up, set related bools false
		m_toolInputCommands.cameraDrag = false;

		m_toolInputCommands.rotRight = false;
		m_toolInputCommands.rotLeft = false;
		m_toolInputCommands.rotUp = false;
		m_toolInputCommands.rotDown = false;

		break;

	}
	//here we update all the actual app functionality that we want.  This information will either be used int toolmain, or sent down to the renderer (Camera movement etc
	//WASD movement
	if (m_keyArray['W'])
	{
		m_toolInputCommands.forward = true;
	}
	else m_toolInputCommands.forward = false;
	
	if (m_keyArray['S'])
	{
		m_toolInputCommands.back = true;
	}
	else m_toolInputCommands.back = false;
	if (m_keyArray['A'])
	{
		m_toolInputCommands.left = true;
	}
	else m_toolInputCommands.left = false;

	if (m_keyArray['D'])
	{
		m_toolInputCommands.right = true;
	}
	else m_toolInputCommands.right = false;

	//Multiple selections while shift key down
	if (m_keyArray[16])
	{
		m_toolInputCommands.multiPick = true;
	}
	else m_toolInputCommands.multiPick = false;

	//Copy objects with c
	if (m_keyArray['C'])
	{
		m_toolInputCommands.copySelection = true;
	}
	else m_toolInputCommands.copySelection = false;

	//Rotate objects
	//X rotation with R
	if (m_keyArray['R'])
	{
		m_toolInputCommands.rotDragX = true;
	}
	else
	{
		m_toolInputCommands.rotDragX = false;
		m_toolInputCommands.rotObjXPos = false;
		m_toolInputCommands.rotObjXNeg = false;

	}
	//Y rotation with T
	if (m_keyArray['T'])
	{
		m_toolInputCommands.rotDragY = true;
	}
	else
	{
		m_toolInputCommands.rotDragY = false;
		m_toolInputCommands.rotObjYPos = false;
		m_toolInputCommands.rotObjYNeg = false;

	}
	//Z rotation with Y
	if (m_keyArray['Y'])
	{
		m_toolInputCommands.rotDragZ = true;
	}
	else
	{
		m_toolInputCommands.rotDragZ = false;
		m_toolInputCommands.rotObjZPos = false;
		m_toolInputCommands.rotObjZNeg = false;

	}

	//Scale objects
	//X scaling with F
	if (m_keyArray['F'])
	{
		m_toolInputCommands.scaleDragX = true;
	}
	else
	{
		m_toolInputCommands.scaleDragX = false;
		m_toolInputCommands.scaleObjXUp = false;
		m_toolInputCommands.scaleObjXDown = false;
	}
	//Y scaling with G
	if (m_keyArray['G'])
	{
		m_toolInputCommands.scaleDragY = true;
	}
	else
	{
		m_toolInputCommands.scaleDragY = false;
		m_toolInputCommands.scaleObjYUp = false;
		m_toolInputCommands.scaleObjYDown = false;
	}
	//Z scaling with H
	if (m_keyArray['H'])
	{
		m_toolInputCommands.scaleDragZ = true;
	}
	else
	{
		m_toolInputCommands.scaleDragZ = false;
		m_toolInputCommands.scaleObjZUp = false;
		m_toolInputCommands.scaleObjZDown = false;
	}

	//Delete objects with backspace
	if (m_keyArray[8])
	{
		m_toolInputCommands.deleteButton = true;
	}
	else m_toolInputCommands.deleteButton = false;

	//Save objects with enter
	if (m_keyArray[13])
	{
		m_toolInputCommands.saveButton = true;
	}
	else m_toolInputCommands.saveButton = false;

}

void ToolMain::loadSceneGraph(std::vector<DisplayObject>* displayList)
{
	if (!m_sceneGraph.empty())		//is scene graph empty
	{
		m_sceneGraph.clear();		//if not, empty it
	}

	//For each display object, create corresponding scene object
	for (int i = 0; i < displayList->size(); i++)
	{
		SceneObject newSceneObject;

		newSceneObject.ID = displayList->at(i).m_ID;
		newSceneObject.chunk_ID = 0;
		newSceneObject.model_path = displayList->at(i).m_model_path;
		newSceneObject.tex_diffuse_path = displayList->at(i).m_tex_diffuse_path;
		newSceneObject.posX = displayList->at(i).m_position.x;
		newSceneObject.posY = displayList->at(i).m_position.y;
		newSceneObject.posZ = displayList->at(i).m_position.z;
		newSceneObject.rotX = displayList->at(i).m_orientation.x;
		newSceneObject.rotY = displayList->at(i).m_orientation.y;
		newSceneObject.rotZ = displayList->at(i).m_orientation.z;
		newSceneObject.scaX = displayList->at(i).m_scale.x;
		newSceneObject.scaY = displayList->at(i).m_scale.y;
		newSceneObject.scaZ = displayList->at(i).m_scale.z;
		newSceneObject.render = 0;
		newSceneObject.collision = 0;
		newSceneObject.collision_mesh = "";
		newSceneObject.collectable = 0;
		newSceneObject.destructable = 0;
		newSceneObject.health_amount = 0;
		newSceneObject.editor_render = displayList->at(i).m_render;
		newSceneObject.editor_texture_vis = 1;
		newSceneObject.editor_normals_vis = 0;
		newSceneObject.editor_collision_vis = 0;
		newSceneObject.editor_pivot_vis = 0;
		newSceneObject.pivotX = 0;
		newSceneObject.pivotY = 0;
		newSceneObject.pivotZ = 0;
		newSceneObject.snapToGround = 0;
		newSceneObject.AINode = 0;
		newSceneObject.audio_path = "";
		newSceneObject.volume = 0;
		newSceneObject.pitch = 0;
		newSceneObject.pan = 0;
		newSceneObject.one_shot = 0;
		newSceneObject.play_on_init = 0;
		newSceneObject.play_in_editor = 0;
		newSceneObject.min_dist = 0;
		newSceneObject.max_dist = 0;
		newSceneObject.camera = 0;
		newSceneObject.path_node = 0;
		newSceneObject.path_node_start = 0;
		newSceneObject.path_node_end = 0;
		newSceneObject.parent_id = 0;
		newSceneObject.editor_wireframe = displayList->at(i).m_wireframe;
		newSceneObject.name = "Name";

		newSceneObject.light_type = displayList->at(i).m_light_type;
		newSceneObject.light_diffuse_r = displayList->at(i).m_light_diffuse_r;
		newSceneObject.light_diffuse_g = displayList->at(i).m_light_diffuse_g;
		newSceneObject.light_diffuse_b = displayList->at(i).m_light_diffuse_b;
		newSceneObject.light_specular_r = displayList->at(i).m_light_specular_r;
		newSceneObject.light_specular_g = displayList->at(i).m_light_specular_g;
		newSceneObject.light_specular_b = displayList->at(i).m_light_specular_b;
		newSceneObject.light_spot_cutoff = displayList->at(i).m_light_spot_cutoff;
		newSceneObject.light_constant = displayList->at(i).m_light_constant;
		newSceneObject.light_linear = displayList->at(i).m_light_linear;
		newSceneObject.light_quadratic = displayList->at(i).m_light_quadratic;

		//send completed object to scenegraph
		m_sceneGraph.push_back(newSceneObject);
	}
}
