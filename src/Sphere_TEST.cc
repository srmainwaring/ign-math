/*
 * Copyright (C) 2018 Open Source Robotics Foundation
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
 *
*/
#include <gtest/gtest.h>
#include <cmath>

#include "gz/math/Sphere.hh"

using namespace gz;
using namespace math;

/////////////////////////////////////////////////
TEST(SphereTest, Constructor)
{
  // Default constructor
  {
    Sphered sphere;
    EXPECT_DOUBLE_EQ(0.0, sphere.Radius());
    EXPECT_EQ(Material(), sphere.Material());

    Sphered sphere2;
    EXPECT_EQ(sphere, sphere2);
  }

  // Radius constructor
  {
    Sphered sphere(1.0);
    EXPECT_DOUBLE_EQ(1.0, sphere.Radius());
    EXPECT_EQ(Material(), sphere.Material());

    Sphered sphere2(1.0);
    EXPECT_EQ(sphere, sphere2);
  }

  // Radius and mat
  {
    Sphered sphere(1.0, Material(MaterialType::WOOD));
    EXPECT_DOUBLE_EQ(1.0, sphere.Radius());
    EXPECT_EQ(Material(MaterialType::WOOD), sphere.Material());

    Sphered sphere2(1.0, Material(MaterialType::WOOD));
    EXPECT_EQ(sphere, sphere2);
  }
}

//////////////////////////////////////////////////
TEST(SphereTest, Comparison)
{
  const Sphered wood(0.1, Material(MaterialType::WOOD));
  {
    Sphered modified = wood;
    EXPECT_EQ(wood, modified);

    modified.SetRadius(1.0);
    EXPECT_NE(wood, modified);
  }

  {
    Sphered modified = wood;
    EXPECT_EQ(wood, modified);

    modified.SetMaterial(Material(MaterialType::PINE));
    EXPECT_NE(wood, modified);
  }
}

//////////////////////////////////////////////////
TEST(SphereTest, Mutators)
{
  Sphered sphere;
  EXPECT_DOUBLE_EQ(0.0, sphere.Radius());
  EXPECT_EQ(Material(), sphere.Material());

  sphere.SetRadius(.123);
  sphere.SetMaterial(Material(MaterialType::PINE));

  EXPECT_DOUBLE_EQ(.123, sphere.Radius());
  EXPECT_EQ(Material(MaterialType::PINE), sphere.Material());
}

//////////////////////////////////////////////////
TEST(SphereTest, VolumeAndDensity)
{
  double mass = 1.0;
  Sphered sphere(0.001);
  double expectedVolume = (4.0/3.0) * GZ_PI * std::pow(0.001, 3);
  EXPECT_DOUBLE_EQ(expectedVolume, sphere.Volume());

  double expectedDensity = mass / expectedVolume;
  EXPECT_DOUBLE_EQ(expectedDensity, sphere.DensityFromMass(mass));

  // Bad density
  Sphered sphere2;
  EXPECT_GT(0.0, sphere2.DensityFromMass(mass));
  sphere2.SetRadius(1.0);
  EXPECT_GT(0.0, sphere2.DensityFromMass(0.0));
  EXPECT_FALSE(sphere.SetDensityFromMass(0.0));
}

//////////////////////////////////////////////////
TEST(SphereTest, Mass)
{
  double mass = 2.0;
  double r = 0.1;
  Sphered sphere(r);
  EXPECT_TRUE(sphere.SetDensityFromMass(mass));

  MassMatrix3d massMat;
  double ixxIyyIzz = 0.4 * mass * r * r;

  MassMatrix3d expectedMassMat;
  expectedMassMat.SetInertiaMatrix(ixxIyyIzz, ixxIyyIzz, ixxIyyIzz,
      0.0, 0.0, 0.0);
  expectedMassMat.SetMass(mass);

  sphere.MassMatrix(massMat);
  EXPECT_EQ(expectedMassMat, massMat);
  EXPECT_DOUBLE_EQ(expectedMassMat.Mass(), massMat.Mass());
}

//////////////////////////////////////////////////
TEST(SphereTest, VolumeBelow)
{
  double r = 2;
  Sphered sphere(r);

  // Fully below
  {
    Planed _plane(Vector3d{0, 0, 1}, Vector2d(4, 4), 2*r);
    EXPECT_NEAR(sphere.Volume(), sphere.VolumeBelow(_plane), 1e-3);
  }

  // Fully below (because plane is rotated down)
  {
    Planed _plane(Vector3d{0, 0, -1}, Vector2d(4, 4), 2*r);
    EXPECT_NEAR(sphere.Volume(), sphere.VolumeBelow(_plane), 1e-3);
  }

  // Fully above
  {
    Planed _plane(Vector3d{0, 0, 1}, Vector2d(4, 4), -2*r);
    EXPECT_NEAR(sphere.VolumeBelow(_plane), 0, 1e-3);
  }

  // Hemisphere
  {
    Planed _plane(Vector3d{0, 0, 1}, 0);
    EXPECT_NEAR(sphere.Volume() / 2, sphere.VolumeBelow(_plane), 1e-3);
  }

  // Vertical plane
  {
    Planed _plane(Vector3d{1, 0, 0}, 0);
    EXPECT_NEAR(sphere.Volume() / 2, sphere.VolumeBelow(_plane), 1e-3);
  }

  // Expectations from https://planetcalc.com/283/
  {
    Planed _plane(Vector3d{0, 0, 1}, 0.5);
    EXPECT_NEAR(22.90745, sphere.VolumeBelow(_plane), 1e-3);
  }

  {
    Planed _plane(Vector3d{0, 0, 1}, -0.5);
    EXPECT_NEAR(10.60288, sphere.VolumeBelow(_plane), 1e-3);
  }
}

//////////////////////////////////////////////////
TEST(SphereTest, CenterOfVolumeBelow)
{
  double r = 2;
  Sphered sphere(r);

  // Entire sphere below plane
  {
    Planed _plane(Vector3d{0, 0, 1}, Vector2d(0, 0), 2 * r);
    EXPECT_EQ(Vector3d(0, 0, 0), sphere.CenterOfVolumeBelow(_plane));
  }

  // Entire sphere above plane
  {
    Planed _plane(Vector3d{0, 0, 1}, Vector2d(0, 0), -2 * r);
    EXPECT_FALSE(sphere.CenterOfVolumeBelow(_plane).has_value());
  }

  {
    // Halfway point is a good spot to test. Center of Volume for a hemisphere
    // is 3/8 its radius. In this case the point should fall below the y-plane
    Planed _plane(Vector3d{0, 1, 0}, Vector2d(0, 0), 0);
    EXPECT_EQ(
      Vector3d(0, -0.75, 0), sphere.CenterOfVolumeBelow(_plane).value());
  }

  {
    // Halfway point is a good spot to test. Center of Volume for a hemisphere
    // is 3/8 its radius. In this case the point should fall above the y-plane
    // thanks to flipped normal
    Planed _plane(Vector3d{0, -1, 0}, Vector2d(0, 0), 0);
    EXPECT_EQ(
      Vector3d(0, 0.75, 0), sphere.CenterOfVolumeBelow(_plane).value());
  }

  {
    // Handcalculated value.
    // Plane at y = 0.8 pointing upwards
    // Cap height is 2.8
    // Centroid should be at 0.3375. However, keep in mind this assumes an
    // inverted cap.
    // Center of volume below should be at -0.3375
    Planed _plane(Vector3d{0, 1, 0}, Vector2d(0, 0), 0.4 * r);
    EXPECT_EQ(
      Vector3d(0, -0.3375, 0), sphere.CenterOfVolumeBelow(_plane).value());
  }

  {
    // Handcalculated value.
    Planed _plane(Vector3d{0, 1, 0},
      Vector2d(0, 0), -0.4 * r);

    EXPECT_EQ(
      Vector3d(0, -1.225, 0), sphere.CenterOfVolumeBelow(_plane).value());
  }

  {
    // Handcalculated value.
    Planed _plane(Vector3d{0, -1, 0},
      Vector2d(0, 0), -0.4 * r);

    EXPECT_EQ(
      Vector3d(0, 1.225, 0), sphere.CenterOfVolumeBelow(_plane).value());
  }

  {
    // Handcalculated value.
    Planed _plane(Vector3d{0, -1, 0},
      Vector2d(0, 0), 0.4 * r);

    EXPECT_EQ(
      Vector3d(0, 0.3375, 0), sphere.CenterOfVolumeBelow(_plane).value());
  }

  {
    // Weighted sums of the center of volume results in (0,0,0).
    Planed _plane1(Vector3d{0, 0, 1}, -0.5);
    // Flip plane1 axis
    Planed _plane2(Vector3d{0, 0, -1}, -0.5);
    EXPECT_EQ(
      sphere.CenterOfVolumeBelow(_plane1).value() * sphere.VolumeBelow(_plane1)
      + sphere.CenterOfVolumeBelow(_plane2).value()
        * sphere.VolumeBelow(_plane2),
      Vector3d(0, 0, 0)
    );
  }
}
