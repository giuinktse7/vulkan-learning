#ifndef BOUND_BUFFER_H
#define BOUND_BUFFER_H

namespace What
{
  struct BoundBuffer
  {
    VkBuffer buffer;
    VkDeviceMemory bufferMemory;
  };
} // namespace What

#endif