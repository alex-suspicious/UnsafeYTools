#!/usr/bin/env python3
import moderngl
import numpy as np
import imageio
from PIL import Image
import ffmpeg as ffmpeg_python 
import os
import sys 
import argparse

try:
    print(f"Type of ffmpeg_python: {type(ffmpeg_python)}")
    print(f"Module name: {ffmpeg_python.__name__}")
    if hasattr(ffmpeg_python, '__file__'):
        print(f"Module file: {ffmpeg_python.__file__}")
    else:
        print("Module does not have __file__ attribute (might be a built-in or namespace package?).")
    
    has_input = hasattr(ffmpeg_python, 'input')
    has_output = hasattr(ffmpeg_python, 'output')
    has_Error = hasattr(ffmpeg_python, 'Error')
    print(f"Has 'input' attribute: {has_input}")
    print(f"Has 'output' attribute: {has_output}")
    print(f"Has 'Error' attribute: {has_Error}")

except Exception as e:
    print(f"Error during diagnostic checks: {e}")
    print("This also indicates something is fundamentally wrong with the imported 'ffmpeg' module.")

print(f"-------------------------------------")

parser = argparse.ArgumentParser()
parser.add_argument('input_video', help='Input MP4 file')
args = parser.parse_args()

INPUT_VIDEO_PATH = args.input_video
EXTRA_TEXTURE_PATH = "offsets/offset_map.png"
FINAL_OUTPUT_PATH = "temp_output.mp4"
TEMP_VIDEO_ONLY_PATH = "output_video_only.mp4" 
SHADER_PATH = "shader.glsl"

try:
    with open(SHADER_PATH) as f:
        frag_shader = f.read()
except FileNotFoundError:
    print(f"Error: Shader file not found at {SHADER_PATH}")
    exit()

ctx = moderngl.create_standalone_context()

try:
    prog = ctx.program(
        vertex_shader="""
            #version 330
            in vec2 in_pos;
            in vec2 in_uv;
            out vec2 v_uv;
            void main() {
                v_uv = in_uv;
                gl_Position = vec4(in_pos, 0.0, 1.0);
            }
        """,
        fragment_shader=frag_shader,
    )
except moderngl.Error as e:
    print(f"Error compiling shader program: {e}")
    exit()


quad = np.array([
    -1.0, -1.0,  0.0,  0.0,
     1.0, -1.0,  1.0,  0.0,
    -1.0,  1.0,  0.0,  1.0,
     1.0,  1.0,  1.0,  1.0,
], dtype='f4')

vbo  = ctx.buffer(quad.tobytes())
vao  = ctx.simple_vertex_array(prog, vbo, 'in_pos', 'in_uv')

try:
    extra_img = Image.open(EXTRA_TEXTURE_PATH).convert("RGBA")
    extra_tex = ctx.texture(extra_img.size, 4, extra_img.tobytes())
    
    extra_tex.filter = (moderngl.NEAREST, moderngl.NEAREST)
except FileNotFoundError:
     print(f"Error: Extra texture file not found at {EXTRA_TEXTURE_PATH}")
     exit()
except Exception as e:
     print(f"Error loading extra texture: {e}")
     exit()
try:
    reader = imageio.get_reader(INPUT_VIDEO_PATH, format="FFMPEG")
    meta   = reader.get_meta_data()
    fps    = meta.get("fps", 30) 
    width, height = meta.get("size", (640, 480)) 
    if width == 0 or height == 0:
         print("Warning: Could not get video size from metadata. Using default 640x480.")
         width, height = (640, 480)
    video_writer = imageio.get_writer(
        TEMP_VIDEO_ONLY_PATH,
        fps=fps,
        format="FFMPEG",
        codec="libx264",
        quality=8 
    )
except Exception as e:
    print(f"Error opening video file {INPUT_VIDEO_PATH} or setting up writer: {e}")
    exit()

fbo = ctx.simple_framebuffer((width, height))
fbo.use()

print(f"Processing video frames from {INPUT_VIDEO_PATH}...")
frame_count = 0
try:
    for frame in reader:
        frame_height, frame_width, channels = frame.shape
        if (frame_width, frame_height) != (width, height):
             
             print(f"Warning: Frame size mismatch. Expected ({width}, {height}), got ({frame_width}, {frame_height}). Resizing frame data.")
             try:
                 frame_img = Image.fromarray(frame).resize((width, height))
                 frame_data = np.array(frame_img)
                 video_tex = ctx.texture(frame_img.size, channels, frame_data.tobytes())
             except Exception as resize_e:
                 print(f"Error during frame resizing: {resize_e}")
                 continue 
        else:
            video_tex = ctx.texture((width, height), channels, frame.tobytes())
        
        video_tex.filter = (moderngl.NEAREST, moderngl.NEAREST)
        
        video_tex.use(location=0)
        extra_tex.use(location=1)
        prog['video_tex'].value = 0
        prog['extra_tex'].value = 1

        ctx.clear(0.0, 0.0, 0.0, 0.0) 
        vao.render(moderngl.TRIANGLE_STRIP) 

        data = fbo.read(components=3, alignment=1)
        img  = np.frombuffer(data, dtype=np.uint8).reshape((height, width, 3))

        video_writer.append_data(img)

        video_tex.release() 
        frame_count += 1

    print(f"\nFinished processing {frame_count} frames.")

except Exception as e:
    print(f"\nAn error occurred during frame processing: {e}")

    video_writer.close()
    reader.close()

    if os.path.exists(TEMP_VIDEO_ONLY_PATH):
        print(f"Removing partial temporary file {TEMP_VIDEO_ONLY_PATH}...")
        os.remove(TEMP_VIDEO_ONLY_PATH)
        print("Temporary file removed.")
    exit() 

video_writer.close()
reader.close()

print(f"Frame processing complete. Merging video from {TEMP_VIDEO_ONLY_PATH} with audio from {INPUT_VIDEO_PATH} using ffmpeg-python...")

try:
    processed_video_stream = ffmpeg_python.input(TEMP_VIDEO_ONLY_PATH)
    original_audio_stream = ffmpeg_python.input(INPUT_VIDEO_PATH).audio 

    ffmpeg_python.output(
        processed_video_stream.video, 
        original_audio_stream,        
        FINAL_OUTPUT_PATH,
        vcodec='copy', 
        acodec='copy', 
        shortest=None  
    ).run(overwrite_output=True) 

    print(f"Final output saved to {FINAL_OUTPUT_PATH}")

except ffmpeg_python.Error as e:
    print('ffmpeg error during merging:', e.stderr.decode('utf8'))

except Exception as e:
    print(f"An unexpected error occurred during merging: {e}")

finally:
    if os.path.exists(TEMP_VIDEO_ONLY_PATH):
        print(f"Removing temporary file {TEMP_VIDEO_ONLY_PATH}...")
        os.remove(TEMP_VIDEO_ONLY_PATH)
        print("Temporary file removed.")

print("Script finished.")