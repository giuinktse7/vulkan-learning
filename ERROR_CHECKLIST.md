# Error checklist

This document contains a list of errors and suggestions for possible fixes.

## The rendered image flickers on all but one frame

This is likely due to a mistake in frame object initialization. Make sure that
each frame is being initialized correctly.

Example of correct code:

```cpp
for (size_t i = 0; i < g_engine->getMaxFramesInFlight(); ++i)
  {
    VkDescriptorBufferInfo bufferInfo = {};
    // Correctly using frames[i]
    bufferInfo.buffer = frames[i].uniformBuffer.buffer;
    ...
  }
```

Example of incorrect code:

```cpp
for (size_t i = 0; i < g_engine->getMaxFramesInFlight(); ++i)
  {
    VkDescriptorBufferInfo bufferInfo = {};
    // Incorrectly using currentFrame
    bufferInfo.buffer = currentFrame.uniformBuffer.buffer;
    ...
  }
```
