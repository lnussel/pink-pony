#include "cinquo.hh"
#include "Heightmap.hh"

void init(Config& config)
{
    Box3f extent = config.level_size;
    glMatrixMode(GL_PROJECTION);
    glLoadIdentity();
     glOrtho(extent.min.x, extent.max.x,
             extent.min.z, extent.max.z,
             extent.min.y-3, extent.max.y+3);
    glMatrixMode(GL_MODELVIEW);

    cout << extent.min << " " << extent.max << endl;
    
    // Init OpenGL states

    glEnable(GL_DEPTH_TEST);
    glEnable(GL_CULL_FACE);
    glCullFace(GL_BACK);
    glShadeModel(GL_SMOOTH);

    // Init light source
    glEnable(GL_LIGHTING);
    glEnable(GL_LIGHT0);

    glLightfv(GL_LIGHT0, GL_DIFFUSE,
              (GLfloat*)&config.light_diffuse);
    glLightfv(GL_LIGHT0, GL_SPECULAR,
              (GLfloat*)&config.light_specular);
    glLightModelfv(GL_LIGHT_MODEL_AMBIENT,
                   (GLfloat*)&config.light_ambient);

    // TODO: I will likely have to move these material properties..
    
    glMaterialfv(GL_FRONT, GL_SPECULAR,
                 (GLfloat*)&config.heightmap_specular);
    glMaterialfv(GL_FRONT, GL_DIFFUSE,
                 (GLfloat*)&config.heightmap_diffuse);
    glMaterialf(GL_FRONT, GL_SHININESS, config.heightmap_shininess);

}

void draw(Config& config, Heightmap& heightmap)
{
    glPushMatrix();

    glRotatef(90.0,1,0,0);
    glTranslatef(0,-config.level_size.max.y,0);
    

    GLfloat light_dir[] = {config.light_dir.x,
                           config.light_dir.y,
                           config.light_dir.z,
                           0};
    
    glLightfv(GL_LIGHT0, GL_POSITION, light_dir);

    heightmap.draw(&config);
    
    glPopMatrix();   
}

int main(int argc, char** argv)
{
    char* noargv[] = {};
    Config config(0, noargv); 

    if (argc > 1) {
        config.heightmap_file = string(argv[1]);
    }
    
    glfwInit();
    glfwOpenWindowHint(GLFW_FSAA_SAMPLES, 16);

    glfwOpenWindow(config.width, config.height, // width, height
                   0, 0, 0, 0,                  // R, G, B, A
                   0, 0,                        // depth, stencil
                   config.window_mode);         // GLFW_WINDOW | GLFW_FULLSCREEN

    glfwSwapInterval(config.swap_interval);
    
    // We put everything between glfwOpenWindow & glfwTerminate
    // between braces, so that stack variables that need a GL context
    // are destructed, while the context still exists.
    {
        bool running = true;
        
        Heightmap heightmap(config.heightmap_file,
                            config.level_size,
                            config.water_level,
                            config.sand_texture,
                            config.grass_texture);

        glfwSetWindowSize(heightmap.size().x, heightmap.size().y);
        glViewport(0,0,heightmap.size().x, heightmap.size().y);

        init(config);
        
        while(running) {
 
            glClear(GL_COLOR_BUFFER_BIT | GL_DEPTH_BUFFER_BIT);
        
            draw(config, heightmap);

            getErrors();
            calc_fps();
            glfwSwapBuffers();

            // Check if still running
        
            running = running && !glfwGetKey( GLFW_KEY_ESC );
            running = running && !glfwGetKey( 'Q' );
            running = running && glfwGetWindowParam( GLFW_OPENED );
        }
    }
    
    glfwTerminate();

    return 0;   
}
