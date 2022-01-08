/*
 * Copyright 2021 Google Inc. All rights reserved.
 *
 * Licensed under the Apache License, Version 2.0 (the "License");
 * you may not use this file except in compliance with the License.
 * You may obtain a copy of the License at
 *
 *     http://www.apache.org/licenses/LICENSE-2.0
 *
 * Unless required by applicable law or agreed to in writing, software
 * distributed under the License is distributed on an "AS IS" BASIS,
 * WITHOUT WARRANTIES OR CONDITIONS OF ANY KIND, either express or implied.
 * See the License for the specific language governing permissions and
 * limitations under the License.
 */
package com.google.flatbuffers.kotlin

import kotlin.math.max
import kotlin.math.min

/**
 * Represent a chunk of data, where FlexBuffers will be read from.
 */
public interface ReadBuffer {

  /**
   * Scan through the buffer for first byte matching value.
   * @param value to be match
   * @param start inclusive initial position to start searching
   * @param end exclusive final position of the search
   * @return position of a match or -1
   */
  public fun findFirst(value: Byte, start: Int, end: Int = limit): Int

  /**
   * Read boolean from the buffer. Booleans as stored as a single byte
   * @param index position of the element in [ReadBuffer]
   * @return [Boolean] element
   */
  public fun getBoolean(index: Int): Boolean

  /**
   * Read a [Byte] from the buffer.
   * @param index position of the element in [ReadBuffer]
   * @return a byte
   */
  public operator fun get(index: Int): Byte

  /**
   * Read a [UByte] from the buffer.
   * @param index position of the element in [ReadBuffer]
   * @return a [UByte]
   */
  public fun getUByte(index: Int): UByte

  /**
   * Read a [Short] from the buffer.
   * @param index position of the element in [ReadBuffer]
   * @return a [Short]
   */
  public fun getShort(index: Int): Short

  /**
   * Read a [UShort] from the buffer.
   * @param index position of the element in [ReadBuffer]
   * @return a [UShort]
   */
  public fun getUShort(index: Int): UShort

  /**
   * Read a [Int] from the buffer.
   * @param index position of the element in [ReadBuffer]
   * @return an [Int]
   */
  public fun getInt(index: Int): Int

  /**
   * Read a [UInt] from the buffer.
   * @param index position of the element in [ReadBuffer]
   * @return an [UInt]
   */
  public fun getUInt(index: Int): UInt

  /**
   * Read a [Long] from the buffer.
   * @param index position of the element in [ReadBuffer]
   * @return a [Long]
   */
  public fun getLong(index: Int): Long

  /**
   * Read a [ULong] from the buffer.
   * @param index position of the element in [ReadBuffer]
   * @return a [ULong]
   */
  public fun getULong(index: Int): ULong

  /**
   * Read a 32-bit float from the buffer.
   * @param index position of the element in [ReadBuffer]
   * @return a float
   */
  public fun getFloat(index: Int): Float

  /**
   * Read a 64-bit float from the buffer.
   * @param index position of the element in [ReadBuffer]
   * @return a double
   */
  public fun getDouble(index: Int): Double

  /**
   * Read an UTF-8 string from the buffer.
   * @param start initial element of the string
   * @param size size of the string in bytes.
   * @return a `String`
   */
  public fun getString(start: Int = 0, size: Int = limit): String

  /**
   * Read a ByteArray from the buffer.
   * @param start position from the [ReadBuffer] to be read
   * @param length maximum number of bytes to be written in the buffer
   */
  public fun getBytes(array: ByteArray, start: Int, length: Int = array.size)

  /**
   * Expose [ReadBuffer] as an array of bytes.
   * This method is meant to be as efficient as possible, so for a array-backed [ReadBuffer], it should
   * return its own internal data. In case access to internal data is not possible,
   * a copy of the data into an array of bytes might occur.
   * @return [ReadBuffer] as an array of bytes
   */
  public fun data(): ByteArray

  /**
   * Creates a new [ReadBuffer] point to a region of the current buffer, starting at [start] with size [size].
   * @param start starting position of the [ReadBuffer]
   * @param size in bytes of the [ReadBuffer]
   * @return [ReadBuffer] slice.
   */
  public fun slice(start: Int, size: Int): ReadBuffer

  /**
   * Defines the size of the message in the buffer. It also determines last position that buffer
   * can be read. Last byte to be accessed is in position `limit() -1`.
   * @return indicate last position
   */
  public val limit: Int
}

/**
 * Interface to represent a read-write buffers. This interface will be used to access and write FlexBuffer messages.
 */
public interface ReadWriteBuffer : ReadBuffer {
  /**
   * Clears (resets) the buffer so that it can be reused. Write position will be set to the start.
   */
  public fun clear()

  /**
   * Put a [Boolean] into the buffer at [writePosition] . Booleans as stored as single byte.
   * Write position will be incremented.
   * @return [Boolean] element
   */
  public fun put(value: Boolean)

  /**
   * Put an array of bytes into the buffer at [writePosition]. Write position will be incremented.
   * @param value the data to be copied
   * @param start initial position on value to be copied
   * @param length amount of bytes to be copied
   */
  public fun put(value: ByteArray, start: Int = 0, length: Int = value.size)

  /**
   * Put an array of bytes into the buffer at [writePosition]. Write position will be incremented.
   * @param value [ReadBuffer] the data to be copied
   * @param start initial position on value to be copied
   * @param length amount of bytes to be copied
   */
  public fun put(value: ReadBuffer, start: Int = 0, length: Int = value.limit - start)

  /**
   * Write a [Byte] into the buffer at [writePosition]. Write position will be incremented.
   */
  public fun put(value: Byte)

  /**
   * Write a [UByte] into the buffer at [writePosition]. Write position will be incremented.
   */
  public fun put(value: UByte)

  /**
   * Write a [Short] into in the buffer at [writePosition]. Write position will be incremented.
   */
  public fun put(value: Short)

  /**
   * Writea [UShort] into in the buffer at [writePosition]. Write position will be incremented.
   */
  public fun put(value: UShort)

  /**
   * Write a [Int] in the buffer at [writePosition]. Write position will be incremented.
   */
  public fun put(value: Int)

  /**
   * Write a [UInt] into in the buffer at [writePosition]. Write position will be incremented.
   */
  public fun put(value: UInt)

  /**
   * Write a [Long] into in the buffer at [writePosition]. Write position will be
   * incremented.
   */
  public fun put(value: Long)

  /**
   * Write a [ULong] into in the buffer at [writePosition]. Write position will be
   * incremented.
   */
  public fun put(value: ULong)

  /**
   * Write a 32-bit [Float] into the buffer at [writePosition]. Write position will be
   * incremented.
   */
  public fun put(value: Float)

  /**
   * Write a 64-bit [Double] into the buffer at [writePosition]. Write position will be
   * incremented.
   */
  public fun put(value: Double)

  /**
   * Write a [String] encoded as UTF-8 into the buffer at [writePosition]. Write position will be incremented.
   * @return size in bytes of the encoded string
   */
  public fun put(value: CharSequence, encodedLength: Int = -1): Int

  /**
   * Write an array of bytes into the buffer.
   * @param dstIndex initial position where [src] will be copied into.
   * @param src the data to be copied.
   * @param srcStart initial position on [src] that will be copied.
   * @param srcLength amount of bytes to be copied
   */
  public fun set(dstIndex: Int, src: ByteArray, srcStart: Int = 0, srcLength: Int = src.size)

  /**
   * Write an array of bytes into the buffer.
   * @param dstIndex initial position where [src] will be copied into.
   * @param src the data to be copied.
   * @param srcStart initial position on [src] that will be copied.
   * @param srcLength amount of bytes to be copied
   */
  public operator fun set(dstIndex: Int, src: ReadBuffer, srcStart: Int = 0, srcLength: Int)

  /**
   * Write [Boolean] into a given position [index] on the buffer. Booleans as stored as single byte.
   * @param index position of the element in buffer
   */
  public operator fun set(index: Int, value: Boolean)

  /**
   * Write [Byte] into a given position [index] on the buffer.
   * @param index position of the element in the buffer
   */
  public operator fun set(index: Int, value: Byte)

  /**
   * Write [UByte] into a given position [index] on the buffer.
   * @param index position of the element in the buffer
   */
  public operator fun set(index: Int, value: UByte)

  /**
   Short
   * @param index position of the element in [ReadBuffer]
   */
  public fun set(index: Int, value: Short)

  /**
   * Write [UShort] into a given position [index] on the buffer.
   * @param index position of the element in [ReadBuffer]
   */
  public fun set(index: Int, value: UShort)

  /**
   * Write [Int] into a given position [index] on the buffer.
   * @param index position of the element in [ReadBuffer]
   */
  public fun set(index: Int, value: Int)

  /**
   * Write [UInt] into a given position [index] on the buffer.
   * @param index position of the element in [ReadBuffer]
   */
  public fun set(index: Int, value: UInt)

  /**
   * Write [Long] into a given position [index] on the buffer.
   * @param index position of the element in [ReadBuffer]
   */
  public fun set(index: Int, value: Long)

  /**
   * Write [ULong] into a given position [index] on the buffer.
   * @param index position of the element in [ReadBuffer]
   */
  public fun set(index: Int, value: ULong)

  /**
   * Write [Float] into a given position [index] on the buffer.
   * @param index position of the element in [ReadBuffer]
   */
  public fun set(index: Int, value: Float)

  /**
   * Write [Double] into a given position [index] on the buffer.
   * @param index position of the element in [ReadBuffer]
   */
  public fun set(index: Int, value: Double)

  /**
   * Current position of the buffer to be written. It will be automatically updated on [put] operations.
   */
  public var writePosition: Int

  /**
   * Defines the size of the message in the buffer. It also determines last position that buffer
   * can be read or write. Last byte to be accessed is in position `limit() -1`.
   * @return indicate last position
   */
  override val limit: Int

  /**
   * Request capacity of the buffer. In case buffer is already larger
   * than the requested, this method will just return true. Otherwise,
   * It might try to resize the buffer. In case of being unable to allocate
   * enough memory, an exception will be thrown.
   * @param capacity new capacity
   * @param copyAtEnd copy current data at the end of new underlying buffer
   */
  public fun requestCapacity(capacity: Int, copyAtEnd: Boolean = false)

  /**
   * Maximum size in bytes that the buffer supports.
   */
  public val capacity: Int
}

public open class ArrayReadBuffer(protected var buffer: ByteArray,
                                  internal val start: Int = 0,
                                  override val limit: Int = buffer.size - start) : ReadBuffer {

  override fun findFirst(value: Byte, start: Int, end: Int): Int {
    val e = min(end, limit)
    val s = max(0, this.start + start)
    for (i in s until e) if (buffer[i] == value) return i
    return -1
  }

  override fun getBoolean(index: Int): Boolean = buffer[start + index] != 0.toByte()

  override operator fun get(index: Int): Byte = buffer[start + index]

  override fun getUByte(index: Int): UByte = buffer.getUByte(start + index)

  override fun getShort(index: Int): Short = buffer.getShort(start + index)

  override fun getUShort(index: Int): UShort = buffer.getUShort(start + index)

  override fun getInt(index: Int): Int = buffer.getInt(start + index)

  override fun getUInt(index: Int): UInt = buffer.getUInt(start + index)

  override fun getLong(index: Int): Long = buffer.getLong(start + index)

  override fun getULong(index: Int): ULong = buffer.getULong(start + index)

  override fun getFloat(index: Int): Float = buffer.getFloat(start + index)

  override fun getDouble(index: Int): Double = buffer.getDouble(start + index)

  override fun getString(start: Int, size: Int): String = buffer.decodeToString(this.start + start,
    this.start + start + size)

  override fun getBytes(array: ByteArray, start: Int, length: Int) {
    val end = min(this.start + start + length, buffer.size)
    var j = 0
    for (i in this.start + start until end) {
      array[j++] = buffer[i]
    }
  }

  override fun data(): ByteArray = buffer

  override fun slice(start: Int, size: Int): ReadBuffer = ArrayReadBuffer(buffer, this.start + start, size)
}
/**
 * Implements `[ReadWriteBuffer]` using [ByteArray] as backing buffer. Using array of bytes are
 * usually faster than `ByteBuffer`.
 *
 * This class is not thread-safe, meaning that
 * it must operate on a single thread. Operating from
 * multiple thread leads into a undefined behavior
 *
 * All operations assume Little Endian byte order.
 */
public class ArrayReadWriteBuffer(
  buffer: ByteArray,
  start: Int = 0,
  override var writePosition: Int = start
) : ArrayReadBuffer(buffer, start, writePosition), ReadWriteBuffer {

  public constructor(initialCapacity: Int = 10) : this(ByteArray(initialCapacity))

  override val limit: Int get() = writePosition

  override fun clear(): Unit = run { writePosition = 0 }

  override fun put(value: Boolean) {
    set(writePosition, value)
    writePosition++
  }

  override fun put(value: ByteArray, start: Int, length: Int) {
    set(writePosition, value, start, length)
    writePosition += length
  }

  override fun put(value: ReadBuffer, start: Int, length: Int) {
    set(writePosition, value, start, length)
    writePosition += length
  }

  override fun put(value: Byte) {
    set(writePosition, value)
    writePosition++
  }

  override fun put(value: UByte) {
    set(writePosition, value)
    writePosition++
  }

  override fun put(value: Short) {
    set(writePosition, value)
    writePosition += 2
  }

  override fun put(value: UShort) {
    set(writePosition, value)
    writePosition += 2
  }

  override fun put(value: Int) {
    set(writePosition, value)
    writePosition += 4
  }

  override fun put(value: UInt) {
    set(writePosition, value)
    writePosition += 4
  }

  override fun put(value: Long) {
    set(writePosition, value)
    writePosition += 8
  }

  override fun put(value: ULong) {
    set(writePosition, value)
    writePosition += 8
  }

  override fun put(value: Float) {
    set(writePosition, value)
    writePosition += 4
  }

  override fun put(value: Double) {
    set(writePosition, value)
    writePosition += 8
  }

  override fun put(value: CharSequence, encodedLength: Int): Int {
    val length = if (encodedLength != -1) encodedLength else Utf8.encodedLength(value)
    withCapacity(writePosition + length) {
      writePosition = setCharSequence(writePosition, value)
    }
    return length
  }

  override fun set(index: Int, value: Boolean) {
    set(index, if (value) 1.toByte() else 0.toByte())
  }

  override fun set(dstIndex: Int, src: ByteArray, srcStart: Int, srcLength: Int) {
    withCapacity(dstIndex + srcLength + srcStart) {
      src.copyInto(buffer, dstIndex, srcStart, srcStart + srcLength)
    }
  }

  override operator fun set(dstIndex: Int, src: ReadBuffer, srcStart: Int, srcLength: Int) {
    withCapacity(dstIndex + (srcLength + srcStart)) {
      when(src) {
        is ArrayReadBuffer -> {
          src.data().copyInto(buffer, dstIndex, src.start + srcStart, src.start + srcStart + srcLength)
        }
        else -> {
          for (i in 0 until srcLength) {
            buffer[dstIndex + i] = src[srcStart + i]
          }
        }
      }
    }
  }

  override operator fun set(index: Int, value: Byte): Unit = withCapacity(index + 1) { set(index, value) }
  override operator fun set(index: Int, value: UByte): Unit = withCapacity(index + 1) { setUByte(index, value) }
  override operator fun set(index: Int, value: Short): Unit = withCapacity(index + 2) { setShort(index, value) }
  override operator fun set(index: Int, value: UShort): Unit = withCapacity(index + 2) { setUShort(index, value) }
  override operator fun set(index: Int, value: Int): Unit = withCapacity(index + 4) { setInt(index, value) }
  override operator fun set(index: Int, value: UInt): Unit = withCapacity(index + 4) { setUInt(index, value) }
  override operator fun set(index: Int, value: Long): Unit = withCapacity(index + 8) { setLong(index, value) }
  override operator fun set(index: Int, value: ULong): Unit = withCapacity(index + 8) { setULong(index, value) }
  override operator fun set(index: Int, value: Float): Unit = withCapacity(index + 4) { setFloat(index, value) }
  override operator fun set(index: Int, value: Double): Unit = withCapacity(index + 8) { setDouble(index, value) }

  override fun requestCapacity(capacity: Int, copyAtEnd: Boolean) {
    if (capacity < 0) error("Capacity may not be negative (likely a previous int overflow)")

    if (buffer.size >= capacity) return
    // implemented in the same growing fashion as ArrayList
    val oldCapacity = buffer.size
    if (oldCapacity == Int.MAX_VALUE - 8) { // Ensure we don't grow beyond what fits in an int.
      error("FlatBuffers: cannot grow buffer beyond 2 gigabytes.");
    }
    //(old_buf_size & 0xC0000000) != 0 ? MAX_BUFFER_SIZE : old_buf_size << 1;
    var newCapacity = 8
    while (newCapacity < capacity) { // Note: this also catches newCapacity int overflow
      newCapacity = if (newCapacity and -0x40000000 != 0) Int.MAX_VALUE - 8 else newCapacity shl 1
    }
    val newBuffer = ByteArray(newCapacity)

    buffer.copyInto(newBuffer, if (copyAtEnd) newBuffer.size - buffer.size else 0)
    buffer = newBuffer
  }

  override val capacity: Int
    get() = buffer.size

  private inline fun withCapacity(size: Int, crossinline action: ByteArray.() -> Unit) {
    requestCapacity(size)
    buffer.action()
  }
}

public val emptyBuffer: ReadWriteBuffer = ArrayReadWriteBuffer(ByteArray(1))
