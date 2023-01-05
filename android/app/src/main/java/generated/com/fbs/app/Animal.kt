// automatically generated by the FlatBuffers compiler, do not modify

package com.fbs.app

import java.nio.*
import kotlin.math.sign
import com.google.flatbuffers.*

@Suppress("unused")
class Animal : Table() {

    fun __init(_i: Int, _bb: ByteBuffer)  {
        __reset(_i, _bb)
    }
    fun __assign(_i: Int, _bb: ByteBuffer) : Animal {
        __init(_i, _bb)
        return this
    }
    val name : String?
        get() {
            val o = __offset(4)
            return if (o != 0) __string(o + bb_pos) else null
        }
    val nameAsByteBuffer : ByteBuffer get() = __vector_as_bytebuffer(4, 1)
    fun nameInByteBuffer(_bb: ByteBuffer) : ByteBuffer = __vector_in_bytebuffer(_bb, 4, 1)
    val sound : String?
        get() {
            val o = __offset(6)
            return if (o != 0) __string(o + bb_pos) else null
        }
    val soundAsByteBuffer : ByteBuffer get() = __vector_as_bytebuffer(6, 1)
    fun soundInByteBuffer(_bb: ByteBuffer) : ByteBuffer = __vector_in_bytebuffer(_bb, 6, 1)
    val weight : UShort
        get() {
            val o = __offset(8)
            return if(o != 0) bb.getShort(o + bb_pos).toUShort() else 0u
        }
    companion object {
        fun validateVersion() = Constants.FLATBUFFERS_23_1_4()
        fun getRootAsAnimal(_bb: ByteBuffer): Animal = getRootAsAnimal(_bb, Animal())
        fun getRootAsAnimal(_bb: ByteBuffer, obj: Animal): Animal {
            _bb.order(ByteOrder.LITTLE_ENDIAN)
            return (obj.__assign(_bb.getInt(_bb.position()) + _bb.position(), _bb))
        }
        fun createAnimal(builder: FlatBufferBuilder, nameOffset: Int, soundOffset: Int, weight: UShort) : Int {
            builder.startTable(3)
            addSound(builder, soundOffset)
            addName(builder, nameOffset)
            addWeight(builder, weight)
            return endAnimal(builder)
        }
        fun startAnimal(builder: FlatBufferBuilder) = builder.startTable(3)
        fun addName(builder: FlatBufferBuilder, name: Int) = builder.addOffset(0, name, 0)
        fun addSound(builder: FlatBufferBuilder, sound: Int) = builder.addOffset(1, sound, 0)
        fun addWeight(builder: FlatBufferBuilder, weight: UShort) = builder.addShort(2, weight.toShort(), 0)
        fun endAnimal(builder: FlatBufferBuilder) : Int {
            val o = builder.endTable()
            return o
        }
        fun finishAnimalBuffer(builder: FlatBufferBuilder, offset: Int) = builder.finish(offset)
        fun finishSizePrefixedAnimalBuffer(builder: FlatBufferBuilder, offset: Int) = builder.finishSizePrefixed(offset)
    }
}
