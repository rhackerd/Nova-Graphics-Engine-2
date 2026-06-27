# Compile both entry points from their respective files
# -profile glsl_450 targets Vulkan SPIR-V 1.3+
# -emit-spirv-directly skips the glslang intermediate step

slangc shaders/vert.slang -profile glsl_450 -target spirv -entry main -o shaders/vert.spv
slangc shaders/frag.slang -profile glsl_450 -target spirv -entry main -o shaders/frag.spv

# common.slang is picked up automatically via 'import common'
# no need to compile it separately — it's a module not an entry point