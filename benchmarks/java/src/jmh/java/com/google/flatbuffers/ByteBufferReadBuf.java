package com.google.flatbuffers;

import java.nio.ByteBuffer;
import java.nio.ByteOrder;

public class ByteBufferReadBuf implements ReadBuf {

  private final ByteBuffer buffer;

  public ByteBufferReadBuf(ByteBuffer bb) {
    this.buffer = bb;
  }

  @Override
  public boolean getBoolean(int index) {
    return get(index) != 0;
  }

  @Override
  public byte get(int index) {
    return buffer.get(index);
  }

  @Override
  public short getShort(int index) {
    return buffer.getShort(index);
  }

  @Override
  public int getInt(int index) {
    return buffer.getInt(index);
  }

  @Override
  public long getLong(int index) {
    return buffer.getLong(index);
  }

  @Override
  public float getFloat(int index) {
    return buffer.getFloat(index);
  }

  @Override
  public double getDouble(int index) {
    return buffer.getDouble(index);
  }

  @Override
  public String getString(int start, int size) {
    return Utf8Safe.decodeUtf8Buffer(buffer, start, size);
  }

  @Override
  public byte[] data() {
    return buffer.array();
  }

  @Override
  public int limit() {
    return buffer.limit();
  }

}
