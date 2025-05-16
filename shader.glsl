#version 330

in vec2 v_uv;
out vec4 f_color;

uniform sampler2D video_tex;
uniform sampler2D extra_tex;

void main() {
    vec4 shuffle_sample = texture(extra_tex, v_uv);
    vec2 decoded_offset = (shuffle_sample.xy * 2.0) - 1.0;
    vec2 base_new_uv = v_uv + decoded_offset;

    vec4 c = texture(video_tex, base_new_uv);

    gl_FragColor = vec4(1-c.rgb, c.a);
}