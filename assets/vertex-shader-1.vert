#ifdef GL_ES
precision mediump float;
#endif

attribute vec4 a_position;
attribute vec2 a_texCoord;
varying vec2 v_texCoord;

uniform float uThickness;
uniform mat4 mvp_matrix;
uniform mat4 rotate_matrix;
varying mat4 rotate_matrix_2;

void main()
{
    gl_PointSize = 5.0;
    gl_Position = mvp_matrix * rotate_matrix * a_position;
    v_texCoord = a_texCoord;
    rotate_matrix_2 = rotate_matrix;
}