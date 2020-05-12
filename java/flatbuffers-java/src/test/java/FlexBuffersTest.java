import com.google.flatbuffers.ArrayReadWriteBuf;
import com.google.flatbuffers.FlexBuffers;
import com.google.flatbuffers.FlexBuffersBuilder;

import org.junit.Assert;
import org.junit.Test;

import java.nio.ByteBuffer;
import java.util.HashMap;
import java.util.Map;

public class FlexBuffersTest {

  @Test
  public void testDeprecatedTypedVectorString() {
    // tests whether we are able to support reading deprecated typed vector string
    // data is equivalent to [ "abc", "abc", "abc", "abc"]
    byte[] data = new byte[] {0x03, 0x61, 0x62, 0x63, 0x00, 0x03, 0x61, 0x62, 0x63, 0x00,
      0x03, 0x61, 0x62, 0x63, 0x00, 0x03, 0x61, 0x62, 0x63, 0x00, 0x04, 0x14, 0x10,
      0x0c, 0x08, 0x04, 0x3c, 0x01};
    FlexBuffers.Reference ref = FlexBuffers.getRoot(ByteBuffer.wrap(data));
    Assert.assertEquals(ref.getType(), FlexBuffers.FBT_VECTOR_STRING_DEPRECATED);
    Assert.assertTrue(ref.isTypedVector());
    FlexBuffers.Vector vec = ref.asVector();
    for (int i=0; i< vec.size(); i++) {
      Assert.assertEquals("abc", vec.get(i).asString());
    }
  }

  @Test
  public void testSingleElementBoolean() {
    FlexBuffersBuilder builder = new FlexBuffersBuilder(ByteBuffer.allocate(100));
    builder.putBoolean(true);
    ByteBuffer b = builder.finish();
    assert(FlexBuffers.getRoot(b).asBoolean());
  }

  @Test
  public void testSingleElementByte() {
    FlexBuffersBuilder builder = new FlexBuffersBuilder();
    builder.putInt(10);
    ByteBuffer b = builder.finish();
    Assert.assertEquals(10, FlexBuffers.getRoot(b).asInt());
  }

  @Test
  public void testSingleElementShort() {
    FlexBuffersBuilder builder = new FlexBuffersBuilder();
    builder.putInt(Short.MAX_VALUE);
    ByteBuffer b = builder.finish();
    Assert.assertEquals(Short.MAX_VALUE, (short)FlexBuffers.getRoot(b).asInt());
  }

  @Test
  public void testSingleElementInt() {
    FlexBuffersBuilder builder = new FlexBuffersBuilder();
    builder.putInt(Integer.MIN_VALUE);
    ByteBuffer b = builder.finish();
    Assert.assertEquals(Integer.MIN_VALUE, FlexBuffers.getRoot(b).asInt());
  }

  @Test
  public void testSingleElementLong() {
    FlexBuffersBuilder builder = new FlexBuffersBuilder();
    builder.putInt(Long.MAX_VALUE);
    ByteBuffer b = builder.finish();
    Assert.assertEquals(Long.MAX_VALUE, FlexBuffers.getRoot(b).asLong());
  }

  @Test
  public void testSingleElementFloat() {
    FlexBuffersBuilder builder = new FlexBuffersBuilder();
    builder.putFloat(Float.MAX_VALUE);
    ByteBuffer b = builder.finish();
    Assert.assertEquals(Float.compare(Float.MAX_VALUE, (float) FlexBuffers.getRoot(b).asFloat()), 0);
  }

  @Test
  public void testSingleElementDouble() {
    FlexBuffersBuilder builder = new FlexBuffersBuilder();
    builder.putFloat(Double.MAX_VALUE);
    ByteBuffer b = builder.finish();
    Assert.assertEquals(Double.compare(Double.MAX_VALUE, FlexBuffers.getRoot(b).asFloat()), 0);
  }

  @Test
  public void testSingleElementBigString() {
    FlexBuffersBuilder builder = new FlexBuffersBuilder(ByteBuffer.allocate(10000));
    StringBuilder sb = new StringBuilder();

    for (int i=0; i< 3000; i++) {
      sb.append("a");
    }

    builder.putString(sb.toString());
    ByteBuffer b = builder.finish();

    FlexBuffers.Reference r = FlexBuffers.getRoot(b);

    Assert.assertEquals(FlexBuffers.FBT_STRING, r.getType());
    Assert.assertEquals(sb.toString(), r.asString());
  }

  @Test
  public void testSingleElementSmallString() {
    FlexBuffersBuilder builder = new FlexBuffersBuilder(ByteBuffer.allocate(10000));

    builder.putString("aa");
    ByteBuffer b = builder.finish();
    FlexBuffers.Reference r = FlexBuffers.getRoot(b);

    Assert.assertEquals(FlexBuffers.FBT_STRING, r.getType());
    Assert.assertEquals("aa", r.asString());
  }

  @Test
  public void testSingleElementBlob() {
    FlexBuffersBuilder builder = new FlexBuffersBuilder();
    builder.putBlob(new byte[]{5, 124, 118, -1});
    ByteBuffer b = builder.finish();
    FlexBuffers.Reference r = FlexBuffers.getRoot(b);
    byte[] result = r.asBlob().getBytes();
    Assert.assertEquals((byte)5, result[0]);
    Assert.assertEquals((byte)124, result[1]);
    Assert.assertEquals((byte)118, result[2]);
    Assert.assertEquals((byte)-1, result[3]);
  }

  @Test
  public void testSingleElementUByte() {
    FlexBuffersBuilder builder = new FlexBuffersBuilder();
    builder.putUInt(0xFF);
    ByteBuffer b = builder.finish();
    FlexBuffers.Reference r = FlexBuffers.getRoot(b);
    Assert.assertEquals(255, (int)r.asUInt());
  }

  @Test
  public void testSingleElementUShort() {
    FlexBuffersBuilder builder = new FlexBuffersBuilder();
    builder.putUInt(0xFFFF);
    ByteBuffer b = builder.finish();
    FlexBuffers.Reference r = FlexBuffers.getRoot(b);
    Assert.assertEquals(65535, (int)r.asUInt());
  }

  @Test
  public void testSingleElementUInt() {
    FlexBuffersBuilder builder = new FlexBuffersBuilder();
    builder.putUInt(0xFFFF_FFFFL);
    ByteBuffer b = builder.finish();
    FlexBuffers.Reference r = FlexBuffers.getRoot(b);
    Assert.assertEquals(4294967295L, r.asUInt());
  }

  @Test
  public void testSingleFixedTypeVector() {

    int[] ints = new int[]{5, 124, 118, -1};
    float[] floats = new float[]{5.5f, 124.124f, 118.118f, -1.1f};
    String[] strings = new String[]{"This", "is", "a", "typed", "array"};
    boolean[] booleans = new boolean[]{false, true, true, false};


    FlexBuffersBuilder builder = new FlexBuffersBuilder(ByteBuffer.allocate(512),
      FlexBuffersBuilder.BUILDER_FLAG_NONE);

    int mapPos = builder.startMap();

    int vecPos = builder.startVector();
    for (final int i : ints) {
      builder.putInt(i);
    }
    builder.endVector("ints", vecPos, true, false);

    vecPos = builder.startVector();
    for (final float i : floats) {
      builder.putFloat(i);
    }
    builder.endVector("floats", vecPos, true, false);

    vecPos = builder.startVector();
    for (final boolean i : booleans) {
      builder.putBoolean(i);
    }
    builder.endVector("booleans", vecPos, true, false);

    builder.endMap(null, mapPos);


    ByteBuffer b = builder.finish();
    FlexBuffers.Reference r = FlexBuffers.getRoot(b);
    assert(r.asMap().get("ints").isTypedVector());
    assert(r.asMap().get("floats").isTypedVector());
    assert(r.asMap().get("booleans").isTypedVector());
  }

  public static void testSingleElementVector() {
    FlexBuffersBuilder b = new FlexBuffersBuilder();

    int vecPos = b.startVector();
    b.putInt(99);
    b.putString("wow");
    int vecpos2 = b.startVector();
    b.putInt(99);
    b.putString("wow");
    b.endVector(null, vecpos2, false, false);
    b.endVector(null, vecPos, false, false);
    b.finish();

    FlexBuffers.Reference r = FlexBuffers.getRoot(b.getBuffer());
    Assert.assertEquals(FlexBuffers.FBT_VECTOR, r.getType());
    FlexBuffers.Vector vec = FlexBuffers.getRoot(b.getBuffer()).asVector();
    Assert.assertEquals(3, vec.size());
    Assert.assertEquals(99, vec.get(0).asInt());
    Assert.assertEquals("wow", vec.get(1).asString());
    Assert.assertEquals("[ 99, \"wow\" ]", vec.get(2).toString());
    Assert.assertEquals("[ 99, \"wow\", [ 99, \"wow\" ] ]", FlexBuffers.getRoot(b.getBuffer()).toString());
  }

  @Test
  public void testSingleElementMap() {
    FlexBuffersBuilder b = new FlexBuffersBuilder();

    int mapPost = b.startMap();
    b.putInt("myInt", 0x7fffffbbbfffffffL);
    b.putString("myString", "wow");
    b.putString("myString2", "incredible");
    int start = b.startVector();
    b.putInt(99);
    b.putString("wow");
    b.endVector("myVec", start, false, false);

    b.putFloat("double", 0x1.ffffbbbffffffP+1023);
    b.endMap(null, mapPost);
    b.finish();

    FlexBuffers.Reference r = FlexBuffers.getRoot(b.getBuffer());
    Assert.assertEquals(FlexBuffers.FBT_MAP, r.getType());
    FlexBuffers.Map map = FlexBuffers.getRoot(b.getBuffer()).asMap();
    Assert.assertEquals(5, map.size());
    Assert.assertEquals(0x7fffffbbbfffffffL, map.get("myInt").asLong());
    Assert.assertEquals("wow", map.get("myString").asString());
    Assert.assertEquals("incredible", map.get("myString2").asString());
    Assert.assertEquals(99, map.get("myVec").asVector().get(0).asInt());
    Assert.assertEquals("wow", map.get("myVec").asVector().get(1).asString());
    Assert.assertEquals(Double.compare(0x1.ffffbbbffffffP+1023, map.get("double").asFloat()), 0);
    Assert.assertEquals("{ \"double\" : 1.7976894783391937E308, \"myInt\" : 9223371743723257855, \"myString\" : \"wow\", \"myString2\" : \"incredible\", \"myVec\" : [ 99, \"wow\" ] }",
      FlexBuffers.getRoot(b.getBuffer()).toString());
  }

  @Test
  public void testFlexBuferEmpty() {
    FlexBuffers.Blob blob = FlexBuffers.Blob.empty();
    FlexBuffers.Map ary = FlexBuffers.Map.empty();
    FlexBuffers.Vector map = FlexBuffers.Vector.empty();
    FlexBuffers.TypedVector typedAry = FlexBuffers.TypedVector.empty();
    Assert.assertEquals(blob.size(), 0);
    Assert.assertEquals(map.size(), 0);
    Assert.assertEquals(ary.size(), 0);
    Assert.assertEquals(typedAry.size(), 0);
  }

  @Test
  public void testHashMapToMap() {
    int entriesCount = 12;

    HashMap<String, String> source =  new HashMap<>();
    for (int i = 0; i < entriesCount; i++) {
      source.put("foo_param_" + i, "foo_value_" + i);
    }

    FlexBuffersBuilder builder = new FlexBuffersBuilder(1000);
    int mapStart = builder.startMap();
    for (Map.Entry<String, String> entry : source.entrySet()) {
      builder.putString(entry.getKey(), entry.getValue());
    }
    builder.endMap(null, mapStart);
    ByteBuffer bb = builder.finish();
    bb.rewind();

    FlexBuffers.Reference rootReference = FlexBuffers.getRoot(bb);

    Assert.assertTrue(rootReference.isMap());

    FlexBuffers.Map flexMap = rootReference.asMap();

    FlexBuffers.KeyVector keys = flexMap.keys();
    FlexBuffers.Vector values = flexMap.values();

    Assert.assertEquals(entriesCount, keys.size());
    Assert.assertEquals(entriesCount, values.size());

    HashMap<String, String> result =  new HashMap<>();
    for (int i = 0; i < keys.size(); i++) {
      result.put(keys.get(i).toString(), values.get(i).asString());
    }

    Assert.assertEquals(source, result);
  }

  @Test
  public void testBuilderGrowth() {
    FlexBuffersBuilder builder = new FlexBuffersBuilder();
    builder.putString("This is a small string");
    ByteBuffer b = builder.finish();
    Assert.assertEquals("This is a small string", FlexBuffers.getRoot(b).asString());

    FlexBuffersBuilder failBuilder = new FlexBuffersBuilder(ByteBuffer.allocate(1));
    try {
      failBuilder.putString("This is a small string");
      // This should never be reached, it should throw an exception
      // since ByteBuffers do not grow
      assert(false);
    } catch (java.lang.ArrayIndexOutOfBoundsException exception) {
      // It should throw exception
    }
  }

  @Test
  public void testFlexBufferVectorStrings() {
    FlexBuffersBuilder builder = new FlexBuffersBuilder(ByteBuffer.allocate(10000000));

    int size = 3000;
    StringBuilder sb = new StringBuilder();
    for (int i=0; i< size; i++) {
      sb.append("a");
    }

    String text = sb.toString();
    Assert.assertEquals(text.length(), size);

    int pos = builder.startVector();

    for (int i=0; i<size; i++) {
      builder.putString(text);
    }

    try {
      builder.endVector(null, pos, true, false);
      // this should raise an exception as
      // typed vector of string was deprecated
      assert false;
    } catch(FlexBuffers.FlexBufferException fb) {
      // no op
    }
    // we finish the vector again as non-typed
    builder.endVector(null, pos, false, false);

    ByteBuffer b = builder.finish();
    FlexBuffers.Vector v = FlexBuffers.getRoot(b).asVector();

    Assert.assertEquals(v.size(), size);
    for (int i=0; i<size; i++) {
      Assert.assertEquals(v.get(i).asString().length(), size);
      Assert.assertEquals(v.get(i).asString(), text);
    }
  }

  @Test
  public  void testFlexBuffersTest() {
    FlexBuffersBuilder builder = new FlexBuffersBuilder(ByteBuffer.allocate(512),
      FlexBuffersBuilder.BUILDER_FLAG_SHARE_KEYS_AND_STRINGS);
    testFlexBuffersTest(builder);
    int bufferLimit1 = ((ArrayReadWriteBuf) builder.getBuffer()).limit();

    // Repeat after clearing the builder to ensure the builder is reusable
    builder.clear();
    testFlexBuffersTest(builder);
    int bufferLimit2 = ((ArrayReadWriteBuf) builder.getBuffer()).limit();
    Assert.assertEquals(bufferLimit1, bufferLimit2);
  }

  public static void testFlexBuffersUtf8Map() {
    FlexBuffersBuilder builder = new FlexBuffersBuilder(ByteBuffer.allocate(512),
      FlexBuffersBuilder.BUILDER_FLAG_SHARE_KEYS_AND_STRINGS);

    String key0 = "😨 face1";
    String key1 = "😩 face2";
    String key2 = "😨 face3";
    String key3 = "trademark ®";
    String key4 = "€ euro";
    String[] utf8keys = { "😨 face1", "😩 face2", "😨 face3", "trademark ®", "€ euro"};

    int map = builder.startMap();

    for (String utf8key : utf8keys) {
      builder.putString(utf8key, utf8key);  // Testing key and string reuse.
    }
    builder.endMap(null, map);
    builder.finish();

    FlexBuffers.Map m = FlexBuffers.getRoot(builder.getBuffer()).asMap();

    Assert.assertEquals(m.size(), 5);

    FlexBuffers.KeyVector kv = m.keys();
    for (int i=0; i< utf8keys.length; i++) {
      Assert.assertEquals(kv.get(i).toString(), m.get(i).asString());
    }

    Assert.assertEquals(m.get(key0).asString(), utf8keys[0]);
    Assert.assertEquals(m.get(key1).asString(), utf8keys[1]);
    Assert.assertEquals(m.get(key2).asString(), utf8keys[2]);
    Assert.assertEquals(m.get(key3).asString(), utf8keys[3]);
    Assert.assertEquals(m.get(key4).asString(), utf8keys[4]);
  }

  private void testFlexBuffersTest(FlexBuffersBuilder builder) {
    // Write the equivalent of:
    // { vec: [ -100, "Fred", 4.0, false ], bar: [ 1, 2, 3 ], bar3: [ 1, 2, 3 ],
    // foo: 100, bool: true, mymap: { foo: "Fred" } }
    // It's possible to do this without std::function support as well.
    int map1 = builder.startMap();

    int vec1 = builder.startVector();
    builder.putInt(-100);
    builder.putString("Fred");
    builder.putBlob(new byte[]{(byte) 77});
    builder.putBoolean(false);
    builder.putInt(Long.MAX_VALUE);

    int map2 = builder.startMap();
    builder.putInt("test", 200);
    builder.endMap(null, map2);

    builder.putFloat(150.9);
    builder.putFloat(150.9999998);
    builder.endVector("vec", vec1, false, false);

    vec1 = builder.startVector();
    builder.putInt(1);
    builder.putInt(2);
    builder.putInt(3);
    builder.endVector("bar", vec1, true, false);

    vec1 = builder.startVector();
    builder.putBoolean(true);
    builder.putBoolean(false);
    builder.putBoolean(true);
    builder.putBoolean(false);
    builder.endVector("bools", vec1, true, false);

    builder.putBoolean("bool", true);
    builder.putFloat("foo", 100);

    map2 = builder.startMap();
    builder.putString("bar", "Fred");  // Testing key and string reuse.
    builder.putInt("int", -120);
    builder.putFloat("float", -123.0f);
    builder.putBlob("blob", new byte[]{ 65, 67 });
    builder.endMap("mymap", map2);

    builder.endMap(null, map1);
    builder.finish();

    FlexBuffers.Map m = FlexBuffers.getRoot(builder.getBuffer()).asMap();

    Assert.assertEquals(m.size(), 6);

    // test empty (an null)
    Assert.assertEquals(m.get("no_key").asString(), ""); // empty if fail
    Assert.assertEquals(m.get("no_key").asMap(), FlexBuffers.Map.empty()); // empty if fail
    Assert.assertEquals(m.get("no_key").asKey(), FlexBuffers.Key.empty()); // empty if fail
    Assert.assertEquals(m.get("no_key").asVector(), FlexBuffers.Vector.empty()); // empty if fail
    Assert.assertEquals(m.get("no_key").asBlob(), FlexBuffers.Blob.empty()); // empty if fail
    assert(m.get("no_key").asVector().isEmpty()); // empty if fail

    // testing "vec" field
    FlexBuffers.Vector vec = m.get("vec").asVector();
    Assert.assertEquals(vec.size(), 8);
    Assert.assertEquals(vec.get(0).asLong(), (long) -100);
    Assert.assertEquals(vec.get(1).asString(), "Fred");
    Assert.assertTrue(vec.get(2).isBlob());
    Assert.assertEquals(vec.get(2).asBlob().size(), 1);
    Assert.assertEquals(vec.get(2).asBlob().data().get(0), (byte) 77);
    Assert.assertTrue(vec.get(3).isBoolean());   // Check if type is a bool
    Assert.assertFalse(vec.get(3).asBoolean());  // Check if value is false
    Assert.assertEquals(vec.get(4).asLong(), Long.MAX_VALUE);
    Assert.assertTrue(vec.get(5).isMap());
    Assert.assertEquals(vec.get(5).asMap().get("test").asInt(), 200);
    Assert.assertEquals(Float.compare((float)vec.get(6).asFloat(), 150.9f), 0);
    Assert.assertEquals(Double.compare(vec.get(7).asFloat(), 150.9999998), 0);
    Assert.assertEquals((long)0, (long)vec.get(1).asLong()); //conversion fail returns 0 as C++

    // bar vector
    FlexBuffers.Vector tvec = m.get("bar").asVector();
    Assert.assertEquals(tvec.size(), 3);
    Assert.assertEquals(tvec.get(0).asInt(), 1);
    Assert.assertEquals(tvec.get(1).asInt(), 2);
    Assert.assertEquals(tvec.get(2).asInt(), 3);
    Assert.assertEquals(((FlexBuffers.TypedVector) tvec).getElemType(), FlexBuffers.FBT_INT);

    // bools vector
    FlexBuffers.Vector bvec = m.get("bools").asVector();
    Assert.assertEquals(bvec.size(), 4);
    Assert.assertTrue(bvec.get(0).asBoolean());
    Assert.assertFalse(bvec.get(1).asBoolean());
    Assert.assertTrue(bvec.get(2).asBoolean());
    Assert.assertFalse(bvec.get(3).asBoolean());
    Assert.assertEquals(((FlexBuffers.TypedVector) bvec).getElemType(), FlexBuffers.FBT_BOOL);


    Assert.assertEquals((float)m.get("foo").asFloat(), (float) 100, 0.1);
    Assert.assertTrue(m.get("unknown").isNull());

    // mymap vector
    FlexBuffers.Map mymap = m.get("mymap").asMap();
    Assert.assertEquals(mymap.keys().get(0), m.keys().get(0)); // These should be equal by pointer equality, since key and value are shared.
    Assert.assertEquals(mymap.keys().get(0).toString(), "bar");
    Assert.assertEquals(mymap.values().get(0).asString(), vec.get(1).asString());
    Assert.assertEquals(mymap.get("int").asInt(), -120);
    Assert.assertEquals((float)mymap.get("float").asFloat(), -123.0f, 0.1f);
    Assert.assertArrayEquals(mymap.get("blob").asBlob().getBytes(), new byte[]{65, 67});
    Assert.assertEquals(mymap.get("blob").asBlob().toString(), "AC");
    Assert.assertEquals(mymap.get("blob").toString(), "\"AC\"");
  }
}
