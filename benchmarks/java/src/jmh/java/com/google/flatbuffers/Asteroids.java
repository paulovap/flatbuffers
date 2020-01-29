package com.google.flatbuffers;

public class Asteroids {

  public Asteroid[] asteroids;

  public static class Asteroid {
    public String name;
    public int id;
    public String nametype;
    public String recclass;
    public double mass;
    public String fall;
    public String year;
    public double reclat;
    public double reclong;
    public GeoLocation geolocation;
  }

  public static class GeoLocation {
    public String type;
    public double[] coordinates;
  }
}


