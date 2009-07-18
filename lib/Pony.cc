#include "Pony.hh"

Pony::Pony(V2f pos, float angle,
           float speed,
           int up, int down,
           int left, int right,
           Config* config, ParticleSystem* particle_system)
    : pos(pos),
      angle(angle),
      speed(speed),
      slope_angle(0),
      camera_pos(pos-V2f(sin(angle), cos(angle))),
      up(up),
      down(down),
      left(left),
      right(right),
      shader(config->pony_shader),
      mesh(),
      animation("models/Pony-animated.pskeleton"),
      mesh_drawer(&mesh),
      texture(config->pony_texture.c_str()),
	  out(false), out_delay(false), particle_source(particle_system)
{
    bool loaded = load_mesh(mesh, config->pony_mesh);

    if (!loaded) {
        cerr << "Failed to load mesh file " << config->pony_mesh << endl;
    }

    animation.set_animation("Gallop");
}

Pony::Decision PlayerPony::decide(PonyGame* game, int i)
{
    Decision decision = {0.0f, 0.0f};
    float& accel = decision.acceleration;
    float& steer = decision.steer;

    if (glfwGetKey(up) == GLFW_PRESS)
        accel += 1.0;
    if (glfwGetKey(down) == GLFW_PRESS)
        accel -= 1.0;

    if (glfwGetKey(left) == GLFW_PRESS)
        steer += 1.0;
    if (glfwGetKey(right) == GLFW_PRESS)
        steer -= 1.0;

    if (glfwGetJoystickParam(i,GLFW_PRESENT) == GL_TRUE) {

        float axes[10];

        int n = glfwGetJoystickPos(i,axes,10);

        if (n >= 2) {

            if (fabs(axes[0]) < 0.2) axes[0] = 0.0;
            //accel += axes[1];
            steer -= axes[0]*2;
        }

        unsigned char buttons[20];

        int button_count = glfwGetJoystickButtons(i, buttons, 20);

        if (button_count >= 2) {
            if (buttons[0] == GLFW_PRESS)
                accel += 1.0;
            if (buttons[1] == GLFW_PRESS)
                accel -= 1.0;
        }

        if (accel > 1.0) accel = 1.0;
        if (accel < -1.0) accel = -1.0;
        if (steer > 1.0) steer = 1.0;
        if (steer < -1.0) steer = -1.0;

    }

    accel *= game->config()->pony_acceleration;
    steer *= game->config()->pony_turn_speed;

    return decision;
};

Pony::Decision AIPony::decide(PonyGame* game, int i)
{
    Decision decision = {0.0f, 0.0f};
    float& accel = decision.acceleration;
    float& steer = decision.steer;
	V2f intersection;
	V2f dir(sin(angle), cos(angle));
	V2f nextpos = pos + dir * speed * 5;
	Line nextline(pos,nextpos);
	/*
		position actuelle : pos
		pos2 = pos + dir * speed * time_diff
	*/

	if (game->linelist()->intersects(nextline, &intersection) || game->terrain()->below_water(nextpos, game->config()->water_tolerance)) {
		//printf("help");
		accel -= game->config()->pony_acceleration;
		steer -= game->config()->pony_turn_speed;
	} else {
		accel = game->config()->pony_acceleration;
		//steer += 0.1;
	}

    return decision;
}

void Pony::move(PonyGame* game, double timeDiff, int i)
{
    if (!out) {
        Decision decision = this->decide(game, i);

        float accel = decision.acceleration;
        float steer = decision.steer;

        if (accel > game->config()->pony_acceleration)
            accel = game->config()->pony_acceleration;
        if (accel < -game->config()->pony_acceleration)
            accel = -game->config()->pony_acceleration;
        if (steer > game->config()->pony_turn_speed)
            steer = game->config()->pony_turn_speed;
        if (steer < -game->config()->pony_turn_speed)
            steer = -game->config()->pony_turn_speed;

        float min = game->config()->pony_min_speed;
        float max = game->config()->pony_max_speed;

        speed += accel * timeDiff;
        if (speed < min) speed = min;
        if (speed > max) speed = max;

        angle += steer * timeDiff;

        V2f dir(sin(angle), cos(angle));

        pos += dir * speed * timeDiff;

        // Calculate new camera position

        V2f offset = pos - camera_pos;
        camera_pos = pos - offset * (game->config()->camera_distance
                                     / offset.length());

        V3f pos3 = game->terrain()->get_pos(pos,false);
        V3f dir3 = game->terrain()->get_pos(pos+dir,false)-pos3;

        particle_source.set_pos(pos3 + V3f(0,2,0) + dir3 * 4);
        particle_source.set_dir(dir3);
        particle_source.set_rate(pow(speed/5.0,3)
                                 * game->config()->pony_particle_rate / 100.0);
        particle_source.set_color(game->config()->pony_color[i]);

        particle_source.add_time(timeDiff);

        if (speed < 20) {
            animation.set_animation("Trot");
            animation.step_animation(timeDiff * speed / 12.5);
        } else {
            animation.set_animation("Gallop");
            animation.step_animation(timeDiff * speed / 50);
        }

        animation.recalculate();
    }
}

void Pony::set_camera(PonyGame* game, Camera* camera, int i)
{
    V2f dir(sin(angle), cos(angle));
    V3f pony_height(0, game->config()->pony_height, 0);
    V3f camera_height(0, game->config()->camera_height, 0);

    V3f p = pony_height + game->terrain()->get_pos(pos,false);
    V3f c = camera_height + game->terrain()->get_pos(camera_pos,true);

    camera->look_at(c,p);
}

void Pony::draw(PonyGame* game, int i)
{
    if (!out_delay) {

        V3f position = game->terrain()->get_pos(pos,false);

        texture.bind(GL_TEXTURE0);
        shader.bind();

        shader.set_uniform("hemi_pole",
                           game->config()->hemilight_pole);
        shader.set_uniform("hemi_sky",
                           game->config()->hemilight_sky);
        shader.set_uniform("hemi_ground",
                           game->config()->hemilight_ground);
        shader.set_uniform("velvet_coeff",
                           game->config()->pony_velvet_coeff);
        shader.set_uniform("texture", 0);

        animation.set_bone_matrices(shader, "bone_transforms");

        glColor(game->config()->pony_color[i]);
        glPushMatrix();


        V3f slope =
            game->terrain()->get_pos(pos + V2f(sin(angle),
                                               cos(angle)),false)
            - game->terrain()->get_pos(pos,false);


        // We smooth the x angle to reduce shakiness
        slope_angle *= 31./32.;
        slope_angle += asin(slope.y/ slope.length()) / M_PI * 180 * 1./32.;


        glTranslate(position);
        glRotatef(180 + angle * 180.0/M_PI, 0, 1, 0);
        glRotatef(slope_angle,1,0,0);
        glRotatef(270,1,0,0);


        mesh_drawer.draw(&shader);

        glPopMatrix();

        shader.unbind();
        texture.unbind(GL_TEXTURE0);

        if (out) out_delay = true;
    }

}
