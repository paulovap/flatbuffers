package com.google.flatbuffers;


import com.google.flatbuffers.*;
import java.nio.ByteBuffer;
import java.nio.ByteOrder;

class AsteroidParser {

  public static Asteroids deserialize(byte[] data) {
    ByteBuffer buffer = ByteBuffer.wrap(data);
    buffer.order(ByteOrder.LITTLE_ENDIAN);

    FlexBuffers.Reference root = FlexBuffers.getRoot(buffer);
    FlexBuffers.Vector asteroidList = root.asMap().get("asteroids").asVector();

    Asteroids.Asteroid[] asteroids = new Asteroids.Asteroid[asteroidList.size()];
    for (int i=0; i<asteroidList.size(); i++) {

      Asteroids.Asteroid asteroid = new Asteroids.Asteroid();
      FlexBuffers.Map map = asteroidList.get(i).asMap();
      FlexBuffers.Map geoMap = map.get("geolocaiton").asMap();
      if (geoMap.size == 2){
        FlexBuffers.Vector coord = geoMap.get("coordinates").asMap();
        asteroid.geolocation = new Asteroids.GeoLocation();
        asteroid.geolocation.type = geoMap.get("type").asString();
        asteroid.geolocation.coordinates = new double[] {coord.get(0).asFloat(), coord.get(1).asFloat()};
      }
      asteroid.name = map.get("name").asString();
      asteroid.fall = map.get("fall").asString();
      asteroid.id = map.get("id").asInt();
      asteroid.mass = map.get("mass").asFloat();
      asteroid.nametype = map.get("nametype").asString();
      asteroid.recclass = map.get("recclass").asString();
      asteroid.reclat = map.get("reclat").asFloat();
      asteroid.reclong = map.get("reclong").asFloat();
      asteroid.year = map.get("year").asString();

      asteroids[i] = asteroid;
    }
    Asteroids result = new Asteroids();
    result.asteroids = asteroids;
    return result;
  }

  public static Asteroids deserializeReadBuf(byte[] data) {
    FlexBuffersNew.Reference root = FlexBuffersNew.getRoot(new ArrayReadBuf(data, data.length));
    FlexBuffersNew.Vector asteroidList = root.asMap().get("asteroids").asVector();

    Asteroids.Asteroid[] asteroids = new Asteroids.Asteroid[asteroidList.size()];
    for (int i=0; i<asteroidList.size(); i++) {

      Asteroids.Asteroid asteroid = new Asteroids.Asteroid();
      FlexBuffersNew.Map map = asteroidList.get(i).asMap();
      FlexBuffersNew.Map geoMap = map.get("geolocaiton").asMap();
      if (geoMap.size == 2){
        FlexBuffersNew.Vector coord = geoMap.get("coordinates").asMap();
        asteroid.geolocation = new Asteroids.GeoLocation();
        asteroid.geolocation.type = geoMap.get("type").asString();
        asteroid.geolocation.coordinates = new double[] {coord.get(0).asFloat(), coord.get(1).asFloat()};
      }
      asteroid.name = map.get("name").asString();
      asteroid.fall = map.get("fall").asString();
      asteroid.id = map.get("id").asInt();
      asteroid.mass = map.get("mass").asFloat();
      asteroid.nametype = map.get("nametype").asString();
      asteroid.recclass = map.get("recclass").asString();
      asteroid.reclat = map.get("reclat").asFloat();
      asteroid.reclong = map.get("reclong").asFloat();
      asteroid.year = map.get("year").asString();

      asteroids[i] = asteroid;
    }
    Asteroids result = new Asteroids();
    result.asteroids = asteroids;
    return result;
  }
}
