// automatically generated by the FlatBuffers compiler, do not modify

package DictionaryLookup

import com.google.flatbuffers.kotlin.*
@Suppress("unused")
class LongFloatEntry : Table() {

    fun init(i: Int, buffer: ReadWriteBuffer) : LongFloatEntry = reset(i, buffer)
    fun assign(i: Int, buffer: ReadWriteBuffer) : LongFloatEntry = init(i, buffer)

    val key : Long get() = lookupField(4, 0L ) { bb.getLong(it + bufferPos) }

    val value : Float get() = lookupField(6, 0.0f ) { bb.getFloat(it + bufferPos) }

    override fun keysCompare(o1: Offset<*>, o2: Offset<*>, buffer: ReadWriteBuffer) : Int {
        val a = buffer.getLong(offset(4, o1, buffer))
        val b = buffer.getLong(offset(4, o2, buffer))
        return (a - b).toInt().sign()
    }
    companion object {
        fun validateVersion() = VERSION_2_0_8

        fun asRoot(buffer: ReadWriteBuffer) : LongFloatEntry = asRoot(buffer, LongFloatEntry())
        fun asRoot(buffer: ReadWriteBuffer, obj: LongFloatEntry) : LongFloatEntry = obj.assign(buffer.getInt(buffer.limit) + buffer.limit, buffer)


        class LongFloatEntryBuilder(val builder: FlatBufferBuilder) {

            var key : Long
                get() = error("This methods should never be called")
                set(value) = addKey(builder, value)

            var value : Float
                get() = error("This methods should never be called")
                set(value) = addValue(builder, value)
        }
        fun createLongFloatEntry(builder: FlatBufferBuilder, lambda: LongFloatEntryBuilder.() -> Unit = {}) : Offset<LongFloatEntry> {
            val b = LongFloatEntryBuilder(builder)
            startLongFloatEntry(builder)
            b.apply(lambda)
            return endLongFloatEntry(builder)
        }

        fun createLongFloatEntry(builder: FlatBufferBuilder, key: Long, value: Float) : Offset<LongFloatEntry> {
            builder.startTable(2)
            addKey(builder, key)
            addValue(builder, value)
            return endLongFloatEntry(builder)
        }
        fun startLongFloatEntry(builder: FlatBufferBuilder) = builder.startTable(2)

        fun addKey(builder: FlatBufferBuilder, key: Long)  {
            builder.add(key)
            builder.slot(0)
        }

        fun addValue(builder: FlatBufferBuilder, value: Float) = builder.add(1, value, 0.0)

        fun endLongFloatEntry(builder: FlatBufferBuilder) : Offset<LongFloatEntry> {
            val o: Offset<LongFloatEntry> = builder.endTable()
            return o
        }

        fun lookupByKey(obj: LongFloatEntry?, vectorLocation: Int, key: Long, bb: ReadWriteBuffer) : LongFloatEntry? {
            var span = bb.getInt(vectorLocation - 4)
            var start = 0
            while (span != 0) {
                var middle = span / 2
                val tableOffset = indirect(vectorLocation + 4 * (start + middle), bb)
                val value = bb.getLong(offset(4, (bb.capacity - tableOffset).toOffset<Int>(), bb))
                val comp = value.compareTo(key)
                when {
                    comp > 0 -> span = middle
                    comp < 0 -> {
                        middle++
                        start += middle
                        span -= middle
                    }
                    else -> {
                        return (obj ?: LongFloatEntry()).assign(tableOffset, bb)
                    }
                }
            }
            return null
        }
    }
}
