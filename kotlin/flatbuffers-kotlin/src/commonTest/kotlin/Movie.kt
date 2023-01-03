// automatically generated by the FlatBuffers compiler, do not modify

import com.google.flatbuffers.kotlin.*
@Suppress("unused")
class Movie : Table() {

    fun init(i: Int, buffer: ReadWriteBuffer) : Movie = reset(i, buffer)
    fun assign(i: Int, buffer: ReadWriteBuffer) : Movie = init(i, buffer)

    val mainCharacterType : UByte get() = lookupField(4, 0u ) { bb.getUByte(it + bufferPos) }

    fun mainCharacter(obj: Table) : Table? = lookupField(6, null ) { union(obj, it + bufferPos) }

    fun charactersType(j: Int) : UByte = lookupField(8, 0u ) { bb.getUByte(vector(it) + j * 1) }
    val charactersTypeLength : Int get() = lookupField(8, 0 ) { vectorLength(it) }
    fun charactersTypeAsBuffer() : ReadBuffer = vectorAsBuffer(bb, 8, 1)

    fun characters(obj: Table, j: Int) : Table? = lookupField(10, null ) { union(obj, vector(it) + j * 4) }
    val charactersLength : Int get() = lookupField(10, 0 ) { vectorLength(it) }

    companion object {
        fun validateVersion() = VERSION_2_0_8

        fun asRoot(buffer: ReadWriteBuffer) : Movie = asRoot(buffer, Movie())
        fun asRoot(buffer: ReadWriteBuffer, obj: Movie) : Movie = obj.assign(buffer.getInt(buffer.limit) + buffer.limit, buffer)

        fun MovieBufferHasIdentifier(buffer: ReadWriteBuffer) : Boolean = hasIdentifier(buffer, "MOVI")

        class MovieBuilder(val builder: FlatBufferBuilder) {

            var charactersType : ArrayOffset<UByte>
                get() = error("This methods should never be called")
                set(value) = addCharactersType(builder, value)

            var characters : ArrayOffset<Any>
                get() = error("This methods should never be called")
                set(value) = addCharacters(builder, value)
        }
        fun createMovie(builder: FlatBufferBuilder, mainCharacterType: UByte, mainCharacter: Offset<Any>, lambda: MovieBuilder.() -> Unit = {}) : Offset<Movie> {
            val b = MovieBuilder(builder)
            startMovie(builder)
            addMainCharacterType(builder, mainCharacterType)
            addMainCharacter(builder, mainCharacter)
            b.apply(lambda)
            return endMovie(builder)
        }

        fun createMovie(builder: FlatBufferBuilder, mainCharacterType: UByte, mainCharacterOffset: Offset<Any>, charactersTypeOffset: ArrayOffset<UByte>, charactersOffset: ArrayOffset<Any>) : Offset<Movie> {
            builder.startTable(4)
            addCharacters(builder, charactersOffset)
            addCharactersType(builder, charactersTypeOffset)
            addMainCharacter(builder, mainCharacterOffset)
            addMainCharacterType(builder, mainCharacterType)
            return endMovie(builder)
        }
        fun startMovie(builder: FlatBufferBuilder) = builder.startTable(4)

        fun addMainCharacterType(builder: FlatBufferBuilder, mainCharacterType: UByte) = builder.add(0, mainCharacterType, 0)

        fun addMainCharacter(builder: FlatBufferBuilder, mainCharacter: Offset<Any>) = builder.addOffset(1, mainCharacter, null)

        fun addCharactersType(builder: FlatBufferBuilder, charactersType: ArrayOffset<UByte>) = builder.addOffset(2, charactersType, null)

        fun createCharactersTypeVector(builder: FlatBufferBuilder, data: UByteArray) : ArrayOffset<UByte> {
            builder.startVector(1, data.size, 1)
            for (i in data.size - 1 downTo 0) {
                builder.add(data[i])
            }
            return builder.endVector()
        }

        fun startCharactersTypeVector(builder: FlatBufferBuilder, numElems: Int) = builder.startVector(1, numElems, 1)

        fun addCharacters(builder: FlatBufferBuilder, characters: ArrayOffset<Any>) = builder.addOffset(3, characters, null)

        fun createCharactersVector(builder: FlatBufferBuilder, data: Array<Offset<Any>>) : ArrayOffset<Any> {
            builder.startVector(4, data.size, 4)
            for (i in data.size - 1 downTo 0) {
                builder.addOffset(data[i])
            }
            return builder.endVector()
        }

        fun startCharactersVector(builder: FlatBufferBuilder, numElems: Int) = builder.startVector(4, numElems, 4)

        fun endMovie(builder: FlatBufferBuilder) : Offset<Movie> {
            val o: Offset<Movie> = builder.endTable()
                builder.required(o, 6)
            return o
        }

        fun finishMovieBuffer(builder: FlatBufferBuilder, offset: Offset<Movie>) = builder.finish(offset, "MOVI")

        fun finishSizePrefixedMovieBuffer(builder: FlatBufferBuilder, offset: Offset<Movie>) = builder.finishSizePrefixed(offset, "MOVI")
    }
}
