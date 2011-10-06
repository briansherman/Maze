/* maze generation program */

#include <GL/glut.h>
#include <stdlib.h>
#include <stdio.h>
#include <math.h>

/* some useful constants */
#define TRUE 1
#define FALSE 0

/* 2D point structure */
typedef struct {
  float x;
  float y;
} Point2;

/* edge structure */
typedef struct {
  int cell1;    /* edge connects cell1 and cell2 */
  int cell2;
  int vertex1;    /* endpoints of edge - indexes into vertex array */
  int vertex2;
  int valid;    /* edge can be removed */
  int draw;   /* edge should be drawn */
} Edge;

/* global parameters */
int w, h, edges, perimeters, vertices, groups, *group, redges, done=0;
Edge *edge, *perimeter;
Point2 *vertex;
GLfloat wall_width  = .1;
GLfloat wall_height = .3;
GLfloat wall_spacing = .5;

GLdouble lookConstant = 0.1;
GLdouble lookDistance = 10;

GLdouble eyeX = 0;
GLdouble eyeY = 10;
GLdouble eyeZ = .2;

GLdouble lookX = 0;
GLdouble lookY = 0;
GLdouble lookZ = .2;

GLfloat PI = 3.14159265;
GLfloat theta = 3*3.14159265/2;
GLfloat turnSpeed = 3;
GLfloat stepDistance = .1;

int topView = 0;

#define NOR 0
#define EAS 1
#define SOU 2
#define WES 3

int dir = NOR;

/* init_maze initializes a w1 by h1 maze.  all walls are initially
   included.  the edge and perimeter arrays, vertex array, and group
   array are allocated and filled in.  */

void
init_maze(int w1, int h1)
{
  int i, j, vedges, hedges;
  float x, y, t, xoff, yoff;

  vedges = (w1-1)*h1; /* number of vertical edges */
  hedges = (h1-1)*w1; /* number of horizontal edges */
  redges = edges = vedges + hedges;  /* number of removable edges */
  perimeters = 2*w1 + 2*h1;
  vertices = (w1+1)*(h1+1);
  groups = w1*h1;

  /* allocate edge array */
  if ((edge=malloc(edges*sizeof(Edge))) == NULL) {
    fprintf(stderr, "Could not allocate edge table\n");
    exit(1);
  }

  /* fill in the vertical edges */
  for (i=0; i<vedges; i++) {
    x = i%(w1-1); /* convert edge number to column */
    y = i/(w1-1); /* and row */
    j = y*w1 + x; /* convert to cell number */
    edge[i].cell1 = j;
    edge[i].cell2 = j+1;
    edge[i].vertex1 = y*(w1+1) + x+1;   /* convert to vertex number */
    edge[i].vertex2 = (y+1)*(w1+1) + x+1;
    edge[i].valid = TRUE;
    edge[i].draw = TRUE;
  }
  for (i=vedges; i<edges; i++) {
    j = i - vedges; /* convert to cell number */
    x = j%w1;   /* convert edge number to column */
    y = j/w1;   /* and row*/
    edge[i].cell1 = j;
    edge[i].cell2 = j + w1;
    edge[i].vertex1 = (y+1)*(w1+1) + x;   /* convert to vertex number */
    edge[i].vertex2 = (y+1)*(w1+1) + x+1;
    edge[i].valid = TRUE;
    edge[i].draw = TRUE;
  }

  /* allocate perimeter */
  if ((perimeter=malloc(perimeters*sizeof(Edge))) == NULL) {
    fprintf(stderr, "Could not allocate perimeter table\n");
    exit(1);
  }

  /* fill in horizontal perimeter */
  for (i=0; i<w1; i++) {
    perimeter[2*i].cell1 = i;
    perimeter[2*i].cell2 = i;
    perimeter[2*i].vertex1 = i;
    perimeter[2*i].vertex2 = i + 1;
    perimeter[2*i].valid = TRUE;
    perimeter[2*i].draw = TRUE;
    perimeter[2*i+1].cell1 = i + h1*w1;
    perimeter[2*i+1].cell2 = i + h1*w1;
    perimeter[2*i+1].vertex1 = i + h1*(w1+1);
    perimeter[2*i+1].vertex2 = i + h1*(w1+1) + 1;
    perimeter[2*i+1].valid = TRUE;
    perimeter[2*i+1].draw = TRUE;
  }
  /* fill in vertical perimeter */
  for (i=w1; i<w1+h1; i++) {
    j = i-w1;
    perimeter[2*i].cell1 = j*w1;
    perimeter[2*i].cell2 = j*w1;
    perimeter[2*i].vertex1 = j*(w1+1);
    perimeter[2*i].vertex2 = (j+1)*(w1+1);
    perimeter[2*i].valid = TRUE;
    perimeter[2*i].draw = TRUE;
    perimeter[2*i+1].cell1 = (j+1)*w1 - 1;
    perimeter[2*i+1].cell2 = (j+1)*w1 - 1;
    perimeter[2*i+1].vertex1 = (j+1)*(w1+1) - 1;
    perimeter[2*i+1].vertex2 = (j+2)*(w1+1) - 1;
    perimeter[2*i+1].valid = TRUE;
    perimeter[2*i+1].draw = TRUE;
  }

  /* allocate vertex array */
  if ((vertex=malloc(vertices*sizeof(Point2))) == NULL) {
    fprintf(stderr, "Could not allocate vertex table\n");
    exit(1);
  }

  /* determine the required offsets to center the maze using the
     spacing calculated above */
  xoff = -w1*wall_spacing/2;
  yoff = -h1*wall_spacing/2;
  /* fill in the vertex array */
  for (i=0; i<vertices; i++) {
    x = i%(w1+1);
    y = i/(w1+1);
    vertex[i].x = x*wall_spacing + xoff;
    vertex[i].y = y*wall_spacing + yoff;
  }

  /* allocate the group table */
  if ((group=malloc(groups*sizeof(int))) == NULL) {
    fprintf(stderr, "Could not allocate group table\n");
    exit(1);
  }

  /* set the group table to the identity */
  for (i=0; i<groups; i++) {
    group[i] = i;
  }
}

/* this function removes one wall from the maze.  if removing this
   wall connects all cells, an entrance and exit are created and a
   done flag is set */
void
step_maze(void)
{
  int i, j, k, o, n;

  /* randomly select one of the the remaining walls */
  k = rand()%redges;
  /* scan down the edge array till we find the kth removeable edge */
  for (i=0; i<edges; i++) {
    if (edge[i].valid == TRUE) {
      if (k == 0) {
        edge[i].valid = FALSE;
        n = group[edge[i].cell1];
        o = group[edge[i].cell2];
        /* if the cells are already connected don't remove the wall */
        if (n != o) {
          edge[i].draw = FALSE;
          done = 1;
          /* fix up the group array */
          for (j=0; j<groups; j++) {
            if (group[j] == o) {
              group[j] = n;
            }
            if (group[j] != n) {
              done = 0;     /* if we have more than one
                               group we're not done */
            }
          }
        }
        break;
      }
      else {
        k--;
      }
    }
  }
  redges--; /* decriment the number of removable edges */
  /* if we're done, create an entrance and exit */
  if (done) {
    for (j=0; j<2; j++) {
      /* randomly select a perimeter edge */
      k = rand()%(perimeters-j);
      for (i=0; i<perimeters; i++) {
        if (k == 0) {
          if (perimeter[i].valid == TRUE) {
            perimeter[i].draw = FALSE;
            break;
          }
        }
        else {
          k--;
        }
      }
    }
  }
}

void draw_wall(GLfloat x1, GLfloat y1, GLfloat x2, GLfloat y2){
  float length = sqrt(pow(y2 - y1,2) + pow(x2 - x1,2));
  GLfloat xwidth = wall_width*(y2 - y1)/length;
  GLfloat ywidth = wall_width*(x1 - x2)/length;
  glBegin(GL_QUADS);
  
    glColor3f(1,0,0);
    //base
    glNormal3f(0.0, 0.0, -1.0);
    glVertex3f(x1,y1,0);
    glVertex3f(x2,y2,0);
    glVertex3f(x2+xwidth,y2+ywidth,0);
    glVertex3f(x1+xwidth,y1+ywidth,0);
    
    //ceiling
    glNormal3f(0.0, 0.0, 1.0);
    glVertex3f(x1,y1,wall_height);
    glVertex3f(x2,y2,wall_height);
    glVertex3f(x2+xwidth,y2+ywidth,wall_height);
    glVertex3f(x1+xwidth,y1+ywidth,wall_height);
    
    glColor3f(0,1,0);
    //left wall
    glNormal3f((y1 - y2)/length, (x2 - x1)/length, 0.0);
    glVertex3f(x1,y1,0);
    glVertex3f(x1,y1,wall_height);
    glVertex3f(x2,y2,wall_height);
    glVertex3f(x2,y2,0);
    
    //right wall
    glNormal3f((y2 - y1)/length, (x1 - x2)/length, 0.0);
    glVertex3f(x1+xwidth,y1+ywidth,0);
    glVertex3f(x1+xwidth,y1+ywidth,wall_height);
    glVertex3f(x2+xwidth,y2+ywidth,wall_height);
    glVertex3f(x2+xwidth,y2+ywidth,0);
    
    glColor3f(0,0,1);
    //front
    glNormal3f((x1 - x2)/length, (y1 - y2)/length, 0.0);
    glVertex3f(x1,y1,0);
    glVertex3f(x1,y1,wall_height);
    glVertex3f(x1+xwidth,y1+ywidth,wall_height);
    glVertex3f(x1+xwidth,y1+ywidth,0);
    
    //back
    glNormal3f((x2 - x1)/length, (y2 - y1)/length, 0.0);
    glVertex3f(x2, y2, 0);
    glVertex3f(x2, y2, wall_height);
    glVertex3f(x2+xwidth, y2+ywidth, wall_height);
    glVertex3f(x2+xwidth, y2+ywidth, 0);
    
  glEnd();

}

void
draw_maze(void)
{
  Point2 *p;
  int i, j;
  GLfloat x1, y1;
  /* draw the interior edges */
  for (i=0; i<edges; i++) {
    if (edge[i].draw == TRUE) {
      p = vertex + edge[i].vertex1;
      x1 = p->x;
      y1 = p->y;
      p = vertex + edge[i].vertex2;
      draw_wall(x1,y1,p->x,p->y);
    }
  }
  /* draw the perimeter edges */
  for (i=0; i<perimeters; i++) {
    if (perimeter[i].draw == TRUE) {
      p = vertex + perimeter[i].vertex1;
      x1 = p->x;
      y1 = p->y;
      p = vertex + perimeter[i].vertex2;
      draw_wall(x1,y1,p->x,p->y);
    }
  }
  
  float xstart = -(w*wall_spacing)/2;
  float ystart = -(h*wall_spacing)/2;
  //Draw Floor
  for(i=0;i<w;i++){
    for(j=0;j<h;j++){
    glBegin(GL_QUADS);
      glNormal3f(0.0, 0.0, 1.0);
      glVertex3f(xstart + wall_spacing*i, ystart + wall_spacing*j, 0);
      glVertex3f(xstart + wall_spacing*i, ystart + wall_spacing*(j + 1), 0);
      glVertex3f(xstart + wall_spacing*(i + 1), ystart + wall_spacing*(j + 1), 0);
      glVertex3f(xstart + wall_spacing*(i + 1), ystart + wall_spacing*j, 0);
    glEnd();
    }
  }
}



/* standard display function */
void
display(void)
{ 
  glClear(GL_COLOR_BUFFER_BIT | GL_DEPTH_BUFFER_BIT);
  draw_maze();
  glutSwapBuffers();
}

void
myinit()
{
  GLfloat mat_specular[]={0.5, 0.5, 0.5, 1.0};
  GLfloat mat_diffuse[]={0.0, 0.5, 0.0, 1.0};
  GLfloat mat_ambient[]={1.0, 1.0, 1.0, 1.0};
  GLfloat mat_shininess=500.0;
  GLfloat light0_ambient[]={0.0, 0.0, 0.0, 1.0};
  GLfloat light0_diffuse[]={0.5, 0.5, 0.5, 1.0};
  GLfloat light0_specular[]={1.0, 1.0, .0, 1.0};

  GLfloat light0_position[4];

  light0_position[0] = 0.0;
  light0_position[1] = 0.0;
  light0_position[2] = 15.0;
  light0_position[3] = 1.0;

  /* set up ambient, diffuse, and specular components for light 0 */
  glLightfv(GL_LIGHT0, GL_AMBIENT, light0_ambient);
  glLightfv(GL_LIGHT0, GL_DIFFUSE, light0_diffuse);
  glLightfv(GL_LIGHT0, GL_SPECULAR, light0_specular);
  
  /* define material properties for front face of all polygons */
  glMaterialfv(GL_FRONT, GL_SPECULAR, mat_specular);
  glMaterialfv(GL_FRONT, GL_AMBIENT, mat_ambient);
  glMaterialfv(GL_FRONT, GL_DIFFUSE, mat_diffuse);
  glMaterialf(GL_FRONT, GL_SHININESS, mat_shininess);
  
  glShadeModel(GL_SMOOTH); /* enable smooth shading */
  glEnable(GL_LIGHTING); /* enable lighting */
  glEnable(GL_LIGHT0);  /* enable light 0 */
  glEnable(GL_DEPTH_TEST); /* enable z buffer */
  glClearColor (0.0, 0.0, 0.0, 1.0);
  
  // initialize the projection stack
  glMatrixMode(GL_PROJECTION);
  glLoadIdentity();
  gluPerspective(60.0, 1.0, 0.1, 100);
  glMatrixMode(GL_MODELVIEW);
  glLoadIdentity();
  gluLookAt(0.0, 0.0, 15.0, 0.0, 0.0, 9.0, 0.0, 1.0, 0.0);
  glLightfv(GL_LIGHT0, GL_POSITION, light0_position);
  //glRotatef(-45,1,0,0);
  /* build maze */
  init_maze(w, h);
  while (!done) {
    /* remove one edge */
    step_maze();
  }
}

void
mouse(int btn, int state, int x, int y)
{
  /* create a new maze */
  if ((btn==GLUT_LEFT_BUTTON) && (state==GLUT_DOWN)) {
    done = 0;
    init_maze(w, h);
    while (!done) {
      step_maze();
    }
    display();
  }

  /* exit */
  if ((btn==GLUT_RIGHT_BUTTON) && (state==GLUT_DOWN)) {
    exit(1);
  }
}

void
moveInDirection(int direction) {
	if (!topView) {
	  eyeX = eyeX + stepDistance*direction*cos(theta);
	  eyeY = eyeY + stepDistance*direction*sin(theta);
	}
}

void turnToDirection(float theta) {
		lookX = eyeX + sin(theta);
		lookY = eyeY + cos(theta);
}

void
keyboard(unsigned char key, int x, int y)
{
	switch(key){
	case 'w':
		moveInDirection(1);
		break;
	case 'a':
	  if(theta < 2*PI){
	    theta = theta + turnSpeed*PI/180;
	  }else{
	    theta = 0;
	  }
		break;
	case 's':
		moveInDirection(-1);
		break;
	case 'd':
	  if(theta > 0){
	    theta = theta - turnSpeed*PI/180;
	  }else{
	    theta = 359*PI/180;
	  }
		break;
	case 'z':
		lookDistance -= .5;
		turnToDirection(dir);
		break;
	case 'o':
		lookDistance += 0.5;
		turnToDirection(dir);
		break;
	case 't':
		if (!topView) {
			glLoadIdentity();
			gluLookAt(0.0, 0.0, 15.0, 0.0, 0.0, 9.0, 0.0, 1.0, 0.0);
			topView = 1;
		} else {
			topView = 0;
		}
		break;
	case 27:
	  // exit if esc is pushed
	  exit(0);
	  break;
	}
	if (!topView) {
		glLoadIdentity();
		gluLookAt(eyeX,eyeY,eyeZ,eyeX + cos(theta),eyeY + sin(theta),lookZ,0.0,0.0,1.0);
	}
	glutPostRedisplay();
}


int
main(int argc, char **argv)
{
  /* check that there are sufficient arguments */
  if (argc < 3) {
    fprintf(stderr, "The width and height must be specified as command line arguments\n");
    exit(1);
  }
  w = atoi(argv[1]);
  h = atof(argv[2]);

  /* standard initialization */
  glutInit(&argc, argv);
  glutInitDisplayMode(GLUT_DOUBLE | GLUT_RGB | GLUT_DEPTH);
  glutInitWindowSize(500, 500);
  glutCreateWindow("Maze");
  glutMouseFunc(mouse);
  glutKeyboardFunc(keyboard);
  glutDisplayFunc(display);
  glEnable(GL_DEPTH_TEST);
  myinit();
  glutMainLoop();

  return 0;
}
