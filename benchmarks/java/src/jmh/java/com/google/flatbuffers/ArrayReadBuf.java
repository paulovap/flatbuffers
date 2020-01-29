package com.google.flatbuffers;

/**
 * Implements {@code ReadBuf} using an array of bytes
 * as a backing storage. Using array of bytes are
 * usually faster than {@code ByteBuffer}.
 */
public class ArrayReadBuf implements ReadBuf {

  private final byte[] buffer;
  private final int limit;

  public ArrayReadBuf(byte[] buffer, int limit) {
    this.buffer = buffer;
    this.limit = limit;
  }

  @Override
  public boolean getBoolean(int index) {
    return buffer[index] != 0;
  }

  @Override
  public byte get(int index) {
    return buffer[index];
  }

  @Override
  public short getShort(int index) {
    return (short) ((buffer[index+ 1] << 8) | (buffer[index] & 0xff));
  }

  @Override
  public int getInt(int index) {
    return (((buffer[index + 3]) << 24) |
      ((buffer[index + 2] & 0xff) << 16) |
      ((buffer[index + 1] & 0xff) << 8) |
      ((buffer[index] & 0xff)));
  }

  @Override
  public long getLong(int index) {
    return ((((long) buffer[index++] & 0xff)) |
      (((long) buffer[index++] & 0xff) << 8) |
      (((long) buffer[index++] & 0xff) << 16) |
      (((long) buffer[index++] & 0xff) << 24) |
      (((long) buffer[index++] & 0xff) << 32) |
      (((long) buffer[index++] & 0xff) << 40) |
      (((long) buffer[index++] & 0xff) << 48) |
      (((long) buffer[index]) << 56));
  }

  @Override
  public float getFloat(int index) {
    return Float.intBitsToFloat(getInt(index));
  }

  @Override
  public double getDouble(int index) {
    return Double.longBitsToDouble(getLong(index));
  }

  @Override
  public String getString(int start, int size) {
    return Utf8Safe.decodeUtf8Array(buffer, start, size);
  }

  @Override
  public byte[] data() {
    return buffer;
  }

  @Override
  public int limit() {
    return limit;
  }
}
