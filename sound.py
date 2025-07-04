import argparse
import numpy as np
import os
import tempfile
import subprocess
from moviepy.editor import VideoFileClip, AudioFileClip
import soundfile as sf

try:
    with open("accepted", "r") as file:
        print("Accepted")
except FileNotFoundError:
    exit()

def generate_sine_value(freq, t, amplitude=0.1):
    return amplitude * np.sin(2 * np.pi * freq * t)

def extract_audio(input_video):
    print(f"Extracting audio from {input_video}...")
    video = VideoFileClip(input_video)

    if not video.audio:
        raise ValueError("Input video does not have an audio track")

    temp_audio = tempfile.NamedTemporaryFile(suffix='.wav', delete=False)
    video.audio.write_audiofile(temp_audio.name, codec='pcm_s16le', logger=None)
    video.close()
    return temp_audio.name

def add_sine_wave_to_audio(audio_file, frequency, amplitude=0.3):
    print(f"Modifying audio with {frequency}Hz sine wave...")

    audio_data, sample_rate = sf.read(audio_file, dtype='float32')

    if audio_data.ndim == 1:
        n_channels = 1
        n_frames = len(audio_data)
        audio_data = audio_data.reshape(-1, 1)
    else:
        n_frames, n_channels = audio_data.shape

    t = np.arange(n_frames) / sample_rate

    sine_wave_component2 = generate_sine_value(frequency+6000, t, amplitude)
    sine_wave_component3 = generate_sine_value(frequency+15000, t, amplitude)

    total_sine_wave = sine_wave_component2 + sine_wave_component3

    if n_channels > 1:
        total_sine_wave = np.tile(total_sine_wave[:, np.newaxis], (1, n_channels))

    modified_audio_float = (audio_data * 0.012) + total_sine_wave

    max_val = np.max(np.abs(modified_audio_float))
    if max_val > 1.0:
        modified_audio_float = modified_audio_float / max_val

    temp_output = tempfile.NamedTemporaryFile(suffix='.wav', delete=False)
    sf.write(temp_output.name, modified_audio_float, sample_rate, subtype='PCM_16')

    return temp_output.name

def combine_video_audio(input_video, audio_file, output_video):
    print(f"Combining video with new audio to create {output_video}...")

    video = VideoFileClip(input_video)
    new_audio = AudioFileClip(audio_file)
    video = video.set_audio(new_audio)

    video.write_videofile(output_video, codec='libx264', audio_codec='aac', logger=None)

    video.close()
    new_audio.close()

def main():
    parser = argparse.ArgumentParser(description='Modify an MP4 file\'s audio by mixing a sine wave directly into the samples')
    parser.add_argument('input_video', help='Input MP4 file')
    parser.add_argument('output_video', help='Output MP4 file')
    parser.add_argument('--frequency', type=float, default=600.0, help='Sine wave frequency in Hz (default: 440 Hz)')
    parser.add_argument('--amplitude', type=float, default=0.1, help='Sine wave amplitude (0-1, default: 0.01)')
    args = parser.parse_args()

    if not os.path.isfile(args.input_video):
        print(f"Error: Input file '{args.input_video}' does not exist")
        return 1
    try:
        temp_audio = extract_audio(args.input_video)
        modified_audio = add_sine_wave_to_audio(temp_audio, args.frequency, args.amplitude)

        combine_video_audio(args.input_video, modified_audio, args.output_video)

        print(f"Successfully created {args.output_video} with {args.frequency}Hz sine wave mixed into the audio")

        os.unlink(temp_audio)
        os.unlink(modified_audio)
    except Exception as e:
        print(f"Error: {str(e)}")
        return 1
    return 0

if __name__ == "__main__":
    exit(main())
