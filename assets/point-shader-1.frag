#ifdef GL_ES
precision mediump float;
#endif

uniform sampler2D tex0;

void main()
{
    gl_FragColor = vec4(1.0, 0.0, 0.0, 1.0);
}