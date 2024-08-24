# ImgFS

## Context

The goal of this project is to develop a robust C program centered around a system-level theme. Specifically, we will build an interface-driven application to efficiently manage images within a custom file system, inspired by Facebook's "Haystack" architecture. While not required reading, the Haystack system is described in detail in this [paper by Beaver et al.](https://www.usenix.org/event/osdi10/tech/full_papers/Beaver.pdf).

### Background

Social networks manage hundreds of millions of images, which presents unique challenges. Traditional file systems struggle with such large volumes of files and are not optimized for handling multiple resolutions of the same image (e.g., icon, preview, original size).

The "Haystack" approach addresses these issues by:

- Storing multiple images in a single file.
- Automatically managing different resolutions of the same image.
- Combining data (images) and metadata (image information) in a single file.
- Keeping a copy of metadata in memory for rapid access.

### Advantages

This approach offers several benefits:

- **Reduced File Management**: The number of files handled by the operating system is minimized.
- **Multiple Resolution Management**: Different resolutions of images are managed automatically.
- **Image Deduplication**: Identical images, even if submitted under different names, are not duplicated. This is achieved using a "hash" function (SHA-256 in our case), which generates a unique 256-bit signature for each image. This function is collision-resistant, meaning it's almost impossible for two different images to have the same signature.

## Goals

The main objective is to build an image server inspired by a simplified version of Haystack. The project will be completed in phases:

1. **Phase 1: Command-Line Interface (CLI)**
    - Implement basic functions:
      - List data (metadata, image list).
      - Add a new image.
      - Delete an image.
      - Extract an image in a specified resolution ("original", "small", "thumbnail").
    - These functions will be exposed through a CLI.
    - **Multithreading**: Use multithreading to handle concurrent operations, improving the performance and responsiveness of the system.

2. **Phase 2: Web Server**
    - Develop a web server to distribute images over the network using the HTTP protocol.