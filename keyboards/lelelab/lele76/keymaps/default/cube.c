#include "cube.h"
#include "sin.h"
#include "quantum.h"
#include "progmem.h"


#define XCOORD	0
#define YCOORD	1
#define ZCOORD	2

#define CUBE_NODES	8
#define CUBE_EDGES	12

int8_t nodes[CUBE_NODES][3] = 
{
	{-1,-1,-1}, {-1,-1,1}, {-1,1,-1}, {-1,1,1}, {1,-1,-1}, {1,-1,1}, {1,1,-1}, {1,1,1}
};

int8_t nodes_rotated[CUBE_NODES][3];

const int8_t edges[CUBE_EDGES][2] PROGMEM =
{
	{0, 1}, {1, 3}, {3, 2}, {2, 0}, {4, 5}, {5, 7}, {7, 6}, {6, 4}, {0, 4}, {1, 5}, {2, 6}, {3, 7}
};


void ResizeCube(uint8_t l_CubeSize)
{
    for (uint8_t node = 0; node < CUBE_NODES; node++) {
        for (uint8_t coord = 0; coord < 3; coord++) {
            nodes[node][coord] = nodes[node][coord] * l_CubeSize;
        }
    }
}



void DrawCubeRotated(uint16_t l_AngleX, uint16_t l_AngleY, uint16_t l_AngleZ)
{
    const uint8_t l_Xoffset = 128/4*3;
    const uint8_t l_Yoffset = 64/2;
	uint8_t l_edge, l_n0, l_n1;

	
	RotateCube(l_AngleX, l_AngleY, l_AngleZ);
	
	for (l_edge=0; l_edge<CUBE_EDGES; l_edge++) {
		l_n0 = pgm_read_byte(&edges[l_edge][0]);	// Start node
		l_n1 = pgm_read_byte(&edges[l_edge][1]);	// end node	
		
		oled_draw_line(nodes_rotated[l_n0][XCOORD] + l_Xoffset, nodes_rotated[l_n0][YCOORD]+l_Yoffset, nodes_rotated[l_n1][XCOORD]+l_Xoffset, nodes_rotated[l_n1][YCOORD]+l_Yoffset);
	}
}


void RotateCube(uint16_t l_AngleX, uint16_t l_AngleY, uint16_t l_AngleZ)
{
	uint8_t l_Node;
	float rotx, roty, rotz, rotxx, rotyy, rotzz, rotxxx, rotyyy, rotzzz;
	
	float AngleSineX = SIN(l_AngleX);
	float AngleCosineX = COS(l_AngleX);
	float AngleSineY = SIN(l_AngleY);
	float AngleCosineY = COS(l_AngleY);
	float AngleSineZ = SIN(l_AngleZ);
	float AngleCosineZ = COS(l_AngleZ);	
	
		
	for (l_Node = 0; l_Node<CUBE_NODES; l_Node++) {
		rotz = (float)nodes[l_Node][ZCOORD] * AngleCosineY - (float)nodes[l_Node][XCOORD] * AngleSineY;
		rotx = (float)nodes[l_Node][ZCOORD] * AngleSineY + (float)nodes[l_Node][XCOORD] * AngleCosineY;
		roty = (float)nodes[l_Node][YCOORD];
		
		rotyy = roty * AngleCosineX - rotz * AngleSineX;
		rotzz = roty * AngleSineX + rotz * AngleCosineX;
		rotxx = rotx;
		
		rotxxx = rotxx * AngleCosineZ - rotyy * AngleSineZ;
		rotyyy = rotxx * AngleSineZ + rotyy * AngleCosineZ;
		rotzzz = rotzz;

		//store new vertices values for wireframe drawing
		nodes_rotated[l_Node][XCOORD] = rotxxx;
		nodes_rotated[l_Node][YCOORD] = rotyyy;
		nodes_rotated[l_Node][ZCOORD] = rotzzz;
	}
}
