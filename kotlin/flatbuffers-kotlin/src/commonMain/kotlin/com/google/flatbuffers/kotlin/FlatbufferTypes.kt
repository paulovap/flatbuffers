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

import kotlin.math.min


// / @cond FLATBUFFERS_INTERNAL
/**
 * All tables in the generated code derive from this class, and add their own accessors.
 */
public open class Table() {

  /** Used to hold the position of the `bb` buffer.  */
  public var bb_pos: Int = 0

  /** The underlying ReadWriteBuffer to hold the data of the Table.  */
  public var bb: ReadWriteBuffer = emptyBuffer

  /** Used to hold the vtable position.  */
  private var vtable_start: Int = 0

  /** Used to hold the vtable size.  */
  private var vtable_size: Int = 0

  /**
   * Get the underlying ReadWriteBuffer.
   *
   * @return Returns the Table's ReadWriteBuffer.
   */
  public val ReadWriteBuffer: ReadWriteBuffer
    get() = bb

  /**
   * Look up a field in the vtable.
   *
   * @param vtable_offset An `int` offset to the vtable in the Table's ReadWriteBuffer.
   * @return Returns an offset into the object, or `0` if the field is not present.
   */
  public fun __offset(vtable_offset: Int): Int {
    return if (vtable_offset < vtable_size) bb.getShort(vtable_start + vtable_offset).toInt() else 0
  }

  /**
   * Retrieve a relative offset.
   *
   * @param offset An `int` index into the Table's ReadWriteBuffer containing the relative offset.
   * @return Returns the relative offset stored at `offset`.
   */
  public fun __indirect(offset: Int): Int {
    return offset + bb.getInt(offset)
  }

  /**
   * Create a Java `String` from UTF-8 data stored inside the FlatBuffer.
   *
   * This allocates a new string and converts to wide chars upon each access,
   * which is not very efficient. Instead, each FlatBuffer string also comes with an
   * accessor based on __vector_as_ReadWriteBuffer below, which is much more efficient,
   * assuming your Java program can handle UTF-8 data directly.
   *
   * @param offset An `int` index into the Table's ReadWriteBuffer.
   * @return Returns a `String` from the data stored inside the FlatBuffer at `offset`.
   */
  public fun __string(offset: Int): String {
    return __string(offset, bb)
  }

  /**
   * Get the length of a vector.
   *
   * @param offset An `int` index into the Table's ReadWriteBuffer.
   * @return Returns the length of the vector whose offset is stored at `offset`.
   */
  public fun __vector_len(offset: Int): Int {
    var newOffset = offset
    newOffset += bb_pos
    newOffset += bb.getInt(newOffset)
    return bb.getInt(newOffset)
  }

  /**
   * Get the start data of a vector.
   *
   * @param offset An `int` index into the Table's ReadWriteBuffer.
   * @return Returns the start of the vector data whose offset is stored at `offset`.
   */
  public fun __vector(offset: Int): Int {
    var newOffset = offset
    newOffset += bb_pos
    return newOffset + bb.getInt(newOffset) + Int.SIZE_BYTES // data starts after the length
  }

    /**
     * Get a whole vector as a ReadWriteBuffer.
     *
     * This is efficient, since it only allocates a new [ReadWriteBuffer] object,
     * but does not actually copy the data, it still refers to the same bytes
     * as the original ReadWriteBuffer. Also useful with nested FlatBuffers, etc.
     *
     * @param vector_offset The position of the vector in the byte buffer
     * @param elem_size The size of each element in the array
     * @return The [ReadWriteBuffer] for the array
     */
    public fun __vector_as_bytebuffer(vector_offset: Int, elem_size: Int): ReadWriteBuffer {
        val o = __offset(vector_offset)
        //if (o == 0) return null
        //val bb: ReadWriteBuffer = ((bb as Buffer?).duplicate() as ReadWriteBuffer).order(ByteOrder.LITTLE_ENDIAN)
        //val vectorstart = __vector(o)
        //bb.position(vectorstart)
        //bb.limit(vectorstart + __vector_len(o) * elem_size)
        return bb
    }

    /**
     * Initialize vector as a ReadWriteBuffer.
     *
     * This is more efficient than using duplicate, since it doesn't copy the data
     * nor allocattes a new [ReadWriteBuffer], creating no garbage to be collected.
     *
     * @param bb The [ReadWriteBuffer] for the array
     * @param vector_offset The position of the vector in the byte buffer
     * @param elem_size The size of each element in the array
     * @return The [ReadWriteBuffer] for the array
     */
    public fun __vector_in_bytebuffer(bb: ReadWriteBuffer, vector_offset: Int, elem_size: Int): ReadWriteBuffer {
//        val o = this.__offset(vector_offset)
//        if (o == 0) return null
//        val vectorstart = __vector(o)
//        bb.rewind()
//        bb.limit(vectorstart + __vector_len(o) * elem_size)
//        (bb as Buffer).position(vectorstart)
        return bb
    }

  /**
   * Initialize any Table-derived type to point to the union at the given `offset`.
   *
   * @param t A `Table`-derived type that should point to the union at `offset`.
   * @param offset An `int` index into the Table's ReadWriteBuffer.
   * @return Returns the Table that points to the union at `offset`.
   */
  public fun __union(t: Table, offset: Int): Table {
    return __union(t, offset, bb)
  }

  /**
   * Sort tables by the key.
   *
   * @param offsets An 'int' indexes of the tables into the bb.
   * @param bb A `ReadWriteBuffer` to get the tables.
   */
  public fun sortTables(offsets: IntArray, bb: ReadWriteBuffer) {
    val off = offsets.sortedWith { o1, o2 -> keysCompare(o1, o2, bb) }
    for (i in offsets.indices) offsets[i] = off[i]
  }

  /**
   * Compare two tables by the key.
   *
   * @param o1 An 'Integer' index of the first key into the bb.
   * @param o2 An 'Integer' index of the second key into the bb.
   * @param bb A `ReadWriteBuffer` to get the keys.
   */
  public open fun keysCompare(o1: Int, o2: Int, bb: ReadWriteBuffer): Int {
    return 0
  }

  /**
   * Re-init the internal state with an external buffer `ReadWriteBuffer` and an offset within.
   *
   * This method exists primarily to allow recycling Table instances without risking memory leaks
   * due to `ReadWriteBuffer` references.
   */
  public fun __reset(_i: Int, _bb: ReadWriteBuffer) {
    bb = _bb
    if (bb != emptyBuffer) {
      bb_pos = _i
      vtable_start = bb_pos - bb.getInt(bb_pos)
      vtable_size = bb.getShort(vtable_start).toInt()
    } else {
      bb_pos = 0
      vtable_start = 0
      vtable_size = 0
    }
  }

  /**
   * Resets the internal state with a null `ReadWriteBuffer` and a zero position.
   *
   * This method exists primarily to allow recycling Table instances without risking memory leaks
   * due to `ReadWriteBuffer` references. The instance will be unusable until it is assigned
   * again to a `ReadWriteBuffer`.
   */
  public fun __reset() {
    __reset(0, emptyBuffer)
  }

  public companion object {

    public fun __offset(vtable_offset: Int, offset: Int, bb: ReadWriteBuffer): Int {
      val vtable: Int = bb.capacity - offset
      d("__offset(limit=${bb.capacity}, vtable_offset=$vtable_offset, offset=$offset, vtable=$vtable, pos=${vtable + vtable_offset - bb.getInt(vtable)}")
      return bb.getShort(vtable + vtable_offset - bb.getInt(vtable)) + vtable
    }

    /**
     * Retrieve a relative offset.
     *
     * @param offset An `int` index into a ReadWriteBuffer containing the relative offset.
     * @param bb from which the relative offset will be retrieved.
     * @return Returns the relative offset stored at `offset`.
     */
    public fun __indirect(offset: Int, bb: ReadWriteBuffer): Int {
      return offset + bb.getInt(offset)
    }

    /**
     * Create a Java `String` from UTF-8 data stored inside the FlatBuffer.
     *
     * This allocates a new string and converts to wide chars upon each access,
     * which is not very efficient. Instead, each FlatBuffer string also comes with an
     * accessor based on __vector_as_ReadWriteBuffer below, which is much more efficient,
     * assuming your Java program can handle UTF-8 data directly.
     *
     * @param offset An `int` index into the Table's ReadWriteBuffer.
     * @param bb Table ReadWriteBuffer used to read a string at given offset.
     * @param utf8 decoder that creates a Java `String` from UTF-8 characters.
     * @return Returns a `String` from the data stored inside the FlatBuffer at `offset`.
     */
    public fun __string(offset: Int, bb: ReadWriteBuffer): String {
      var newOffset = offset
      newOffset += bb.getInt(newOffset)
      val length: Int = bb.getInt(newOffset)
      return bb.getString(newOffset + Int.SIZE_BYTES, length)
    }

    /**
     * Initialize any Table-derived type to point to the union at the given `offset`.
     *
     * @param t A `Table`-derived type that should point to the union at `offset`.
     * @param offset An `int` index into the Table's ReadWriteBuffer.
     * @param bb Table ReadWriteBuffer used to initialize the object Table-derived type.
     * @return Returns the Table that points to the union at `offset`.
     */
    public fun __union(t: Table, offset: Int, bb: ReadWriteBuffer): Table {
      t.__reset(__indirect(offset, bb), bb)
      return t
    }

    /**
     * Check if a [ReadWriteBuffer] contains a file identifier.
     *
     * @param bb A `ReadWriteBuffer` to check if it contains the identifier
     * `ident`.
     * @param ident A `String` identifier of the FlatBuffer file.
     * @return True if the buffer contains the file identifier
     */
    public fun __has_identifier(bb: ReadWriteBuffer?, ident: String): Boolean {
      val identifierLength = 4
      if (ident.length != identifierLength) throw AssertionError(
        "FlatBuffers: file identifier must be length " +
          identifierLength
      )
      for (i in 0 until identifierLength) {
        if (ident[i].code.toByte() != bb!![bb.limit + Int.SIZE_BYTES + i]) return false
      }
      return true
    }

    /**
     * Compare two strings in the buffer.
     *
     * @param offsetA An 'int' index of the first string into the bb.
     * @param offsetB An 'int' index of the second string into the bb.
     * @param bb A `ReadWriteBuffer` to get the strings.
     */
    public fun compareStrings(offsetA: Int, offsetB: Int, bb: ReadWriteBuffer): Int {
      var offset_1 = offsetA
      var offset_2 = offsetB
      offset_1 += bb.getInt(offset_1)
      offset_2 += bb.getInt(offset_2)
      val len_1: Int = bb.getInt(offset_1)
      val len_2: Int = bb.getInt(offset_2)
      val startPos_1: Int = offset_1 + Int.SIZE_BYTES
      val startPos_2: Int = offset_2 + Int.SIZE_BYTES
      val len: Int = min(len_1, len_2)
      for (i in 0 until len) {
        if (bb[i + startPos_1] != bb[i + startPos_2]) return bb[i + startPos_1] - bb[i + startPos_2]
      }
      return len_1 - len_2
    }

    /**
     * Compare string from the buffer with the 'String' object.
     *
     * @param offset_1 An 'int' index of the first string into the bb.
     * @param key Second string as a byte array.
     * @param bb A `ReadWriteBuffer` to get the first string.
     */
    public fun compareStrings(offset_1: Int, key: ByteArray, bb: ReadWriteBuffer): Int {
      var offset_1 = offset_1
      offset_1 += bb.getInt(offset_1)
      val len_1: Int = bb.getInt(offset_1)
      val len_2 = key.size
      val startPos_1: Int = offset_1 + Int.SIZE_BYTES
      val len: Int = min(len_1, len_2)
      for (i in 0 until len) {
        if (bb[i + startPos_1] != key[i]) return bb.get(i + startPos_1) - key[i]
      }
      return len_1 - len_2
    }
  }
}

/**
 * All structs in the generated code derive from this class, and add their own accessors.
 */
public open class Struct {
  /** Used to hold the position of the `bb` buffer.  */
  protected var bb_pos: Int = 0

  /** The underlying ByteBuffer to hold the data of the Struct.  */
  protected var bb: ReadWriteBuffer = emptyBuffer

  /**
   * Re-init the internal state with an external buffer `ByteBuffer` and an offset within.
   *
   * This method exists primarily to allow recycling Table instances without risking memory leaks
   * due to `ByteBuffer` references.
   */
  protected fun __reset(_i: Int, _bb: ReadWriteBuffer) {
    bb = _bb
    bb_pos = if (bb != emptyBuffer) {
      _i
    } else {
      0
    }
  }

  /**
   * Resets internal state with a null `ByteBuffer` and a zero position.
   *
   * This method exists primarily to allow recycling Struct instances without risking memory leaks
   * due to `ByteBuffer` references. The instance will be unusable until it is assigned
   * again to a `ByteBuffer`.
   *
   * @param struct the instance to reset to initial state
   */
  private fun __reset() {
    __reset(0, emptyBuffer)
  }
}

public const val FLATBUFFERS_VERSION: String = "2.0"
