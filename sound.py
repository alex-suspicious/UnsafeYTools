import argparse
import subprocess
import os
import tempfile

try:
    with open("accepted", "r") as file:
        print("Accepted")
except FileNotFoundError:
    exit()

def main():
    parser = argparse.ArgumentParser(description='Modify an MP4 file\'s audio by mixing sine waves using FFmpeg.')
    parser.add_argument('input_video', help='Input MP4 file')
    parser.add_argument('output_video', help='Output MP4 file')
    parser.add_argument('--frequency', type=float, default=600.0, help='Base sine wave frequency in Hz (default: 600 Hz)')
    parser.add_argument('--amplitude', type=float, default=0.1, help='Sine wave amplitude (0-1, default: 0.1)')
    args = parser.parse_args()

    if not os.path.isfile(args.input_video):
        print(f"Error: Input file '{args.input_video}' does not exist")
        return 1

    try:
        temp_audio_sine = tempfile.NamedTemporaryFile(suffix='.wav', delete=False)
        temp_audio_mixed = tempfile.NamedTemporaryFile(suffix='.wav', delete=False)

        freq1 = args.frequency + 6000
        freq2 = args.frequency + 15000

        cmd_duration = [
            'ffprobe', '-v', 'error', '-show_entries', 'format=duration', '-of', 'default=noprint_wrappers=1:nokey=1',
            args.input_video
        ]
        duration_str = subprocess.check_output(cmd_duration).decode('utf-8').strip()
        duration = float(duration_str) if duration_str else 0.0

        if duration == 0.0:
            raise ValueError("Could not determine video duration. Is the input video valid?")

        cmd_samplerate = [
            'ffprobe', '-v', 'error', '-select_streams', 'a:0', '-show_entries', 'stream=sample_rate', '-of', 'default=noprint_wrappers=1:nokey=1',
            args.input_video
        ]
        samplerate_str = subprocess.check_output(cmd_samplerate).decode('utf-8').strip()
        sample_rate = int(samplerate_str) if samplerate_str else 44100

        ffmpeg_command = [
            'ffmpeg',
            '-i', args.input_video,
            '-f', 'lavfi',
            '-i', f'sine=frequency={freq1}:r={sample_rate}:d={duration}',
            '-f', 'lavfi',
            '-i', f'sine=frequency={freq2}:r={sample_rate}:d={duration}',
            '-filter_complex',
            f'[0:a]volume=0.03[original_scaled];'
            f'[1:a]volume={args.amplitude}[sine1_vol];'
            f'[2:a]volume={args.amplitude}[sine2_vol];'
            f'[sine1_vol][sine2_vol]amix=inputs=2:duration=longest:weights=1 1[sine_sum];'
            f'[original_scaled][sine_sum]amix=inputs=2:duration=longest:normalize=0[final_audio]',
            '-map', '0:v',
            '-map', '[final_audio]',
            '-c:v', 'copy',
            '-c:a', 'aac',
            '-shortest',
            '-y',
            args.output_video
        ]
        
        subprocess.run(ffmpeg_command, check=True, stdout=subprocess.PIPE, stderr=subprocess.PIPE)

        print(f"Successfully created {args.output_video} with mixed sine waves")

    except subprocess.CalledProcessError as e:
        print(f"FFmpeg Error: {e.stderr.decode('utf-8')}")
        return 1
    except ValueError as e:
        print(f"Error: {str(e)}")
        return 1
    except Exception as e:
        print(f"An unexpected error occurred: {str(e)}")
        return 1
    return 0

if __name__ == "__main__":
    exit(main())