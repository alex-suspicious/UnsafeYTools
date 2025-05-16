#!/usr/bin/env python3
import json
from PIL import Image
import os
import string
import random
import math 

def deterministic_hash(s: str, prime: int = 31, modulus: int = 2**32) -> float:
    h = 0
    for char in s:
        h = (h * prime + ord(char)) % modulus
    
    return h / modulus


def generate_maps_and_token(width: int, height: int, seed: str = None) -> tuple[list[list[tuple[float, float]]], list[list[tuple[float, float]]], str]:
    if width <= 0 or height <= 0:
        raise ValueError("Width and height must be positive integers.")
    
    if seed is None:
         raise ValueError("Seed string is required for deterministic generation.")

    used_seed = seed
    print(f"Using seed: {used_seed}") 

    total_pixels = width * height
    
    start_hash = deterministic_hash(used_seed, 31, 2**32 - 1) 
    step_hash = deterministic_hash(used_seed + "_step", 37, 2**32 - 2) 

    start_angle = start_hash * math.pi * 2.0 
    angle_increment = step_hash * math.pi / max(width, height) 

    indexed_values = []
    for i in range(total_pixels):

        value = math.sin(start_angle + i * angle_increment)
        indexed_values.append((value, i)) 

    indexed_values.sort(key=lambda item: item[0]) 

    pLinearized = [0] * total_pixels
    for k in range(total_pixels):
        original_index = indexed_values[k][1]
        shuffled_index = k 
        pLinearized[original_index] = shuffled_index
    
    pInvLinearized = [0] * total_pixels
    for original_idx in range(total_pixels):
        shuffled_idx = pLinearized[original_idx]
        pInvLinearized[shuffled_idx] = original_idx

    unshuffle_uv_offsets = [[(0.0, 0.0)] * width for _ in range(height)]
    for oy in range(height):
        for ox in range(width):
            original_linear_index = oy * width + ox

            shuffled_linear_index = pLinearized[original_linear_index]

            sx_shuffled = shuffled_linear_index % width
            sy_shuffled = shuffled_linear_index // width 

            offset_x = (sx_shuffled - ox) / float(width)
            offset_y = (sy_shuffled - oy) / float(height)

            unshuffle_uv_offsets[oy][ox] = (offset_x, offset_y)

    shuffle_uv_offsets = [[(0.0, 0.0)] * width for _ in range(height)]
    for ty_shuffled in range(height):
        for tx_shuffled in range(width):
            shuffled_linear_index = ty_shuffled * width + tx_shuffled

            original_linear_index = pInvLinearized[shuffled_linear_index]

            ox_original = original_linear_index % width
            oy_original = original_linear_index // width

            offset_x = (ox_original - tx_shuffled) / float(width)
            offset_y = (oy_original - ty_shuffled) / float(height)

            shuffle_uv_offsets[ty_shuffled][tx_shuffled] = (offset_x, offset_y)
    return shuffle_uv_offsets, unshuffle_uv_offsets, used_seed


def save_offset_map_to_png(offset_map: list[list[tuple[float, float]]], width: int, height: int, filename: str):
    if not offset_map or height != len(offset_map) or (height > 0 and width != len(offset_map[0])):
        raise ValueError("Offset map dimensions do not match width and height.")

    image = Image.new("RGB", (width, height))
    pixels = image.load()

    for y in range(height):
        for x in range(width):
            dx, dy = offset_map[y][x]

            r_val = int(max(0, min(255, ((dx + 1.0) / 2.0) * 255.0)))
            g_val = int(max(0, min(255, ((dy + 1.0) / 2.0) * 255.0)))
            b_val = 0 

            pixels[x, y] = (r_val, g_val, b_val)

    output_dir = os.path.dirname(filename)
    if output_dir and not os.path.exists(output_dir):
        os.makedirs(output_dir)

    image.save(filename)
    print(f"Saved offset map to {filename}")

if __name__ == "__main__":
    image_width = 80
    image_height = 80

    output_directory = "offsets"
    if not os.path.exists(output_directory):
        os.makedirs(output_directory)
        print(f"Created directory: {output_directory}")

    shuffle_output_filename = os.path.join(output_directory, "offset_map.png")
    unshuffle_output_filename = os.path.join(output_directory, "inv_offset_map.png")

    print(f"Generating offset maps and token for a {image_width}x{image_height} image...")

    try:
        test_seed = ''.join(random.choices(string.ascii_uppercase + string.digits, k=20)) 

        shuffle_map_data, unshuffle_map_data, used_seed = generate_maps_and_token(image_width, image_height, seed=test_seed)

        print("\nSaving Shuffle Offset Map...")
        save_offset_map_to_png(shuffle_map_data, image_width, image_height, shuffle_output_filename)

        print("\nSaving Unshuffle Offset Map...")
        save_offset_map_to_png(unshuffle_map_data, image_width, image_height, unshuffle_output_filename)

        print("\n--- Permutation Token (Seed) for JavaScript ---")
        
        print(f"Token (Seed): {used_seed}")
        print(f"Width: {image_width}")
        print(f"Height: {image_height}")
        print("-----------------------------------------------")

        print("\nProcess complete. PNG offset maps have been generated and token/seed printed.")

    except ImportError:
        print("Error: The Pillow library (PIL) is not installed.")
        print("Please install it by running: pip install Pillow")
    except ValueError as e:
        print(f"Input Error: {e}")
    except Exception as e:
        print(f"An unexpected error occurred: {e}")