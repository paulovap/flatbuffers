package com.google.flatbuffers;

import org.apache.commons.io.IOUtils;
import org.openjdk.jmh.annotations.*;
import org.openjdk.jmh.infra.Blackhole;

import java.io.File;
import java.io.IOException;
import java.io.InputStream;
import java.nio.file.Files;
import java.util.concurrent.TimeUnit;

public class BenchmarkRunner {

  @State(Scope.Thread)
  public static class Data {
    public byte[] data;
    @Setup(Level.Trial)
    public void doSetup() throws IOException {
      Class clazz = this.getClass();
      InputStream inputStream = clazz.getResourceAsStream("/asteroids.flexbin");
      data = IOUtils.toByteArray(inputStream);
    }
  }

  public static void main(String[] args) throws Exception {
    org.openjdk.jmh.Main.main(args);
  }

  @Benchmark @BenchmarkMode(Mode.Throughput)
  @OutputTimeUnit(TimeUnit.MILLISECONDS)
  public void flexbuffersMaster(Data data, Blackhole blackhole) {
    Asteroids asteroids = AsteroidParser.deserialize(data.data);
    blackhole.consume(asteroids);
  }

  @Benchmark @BenchmarkMode(Mode.Throughput)
  @OutputTimeUnit(TimeUnit.MILLISECONDS)
  public void flexbuffersReadBuff(Data data, Blackhole blackhole) {
    Asteroids asteroids = AsteroidParser.deserializeReadBuf(data.data);
    blackhole.consume(asteroids);
  }
}
