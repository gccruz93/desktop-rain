package main

import (
	"image/color"
	"math/rand"

	"github.com/hajimehoshi/ebiten/v2"
	"github.com/hajimehoshi/ebiten/v2/vector"
)

type Drop struct {
	x          float32
	y          float32
	size       float32
	thickness  float32
	stroke     float32
	mass       float32
	velocity   float32
	brightness float32
}

func NewDrop() *Drop {
	drop := &Drop{}
	drop.regen()
	return drop
}

func (d *Drop) Draw(screen *ebiten.Image, wind float32) {
	c := color.RGBA{
		R: uint8(0xbb * d.brightness / 0xff),
		G: uint8(0xdd * d.brightness / 0xff),
		B: uint8(0xff * d.brightness / 0xff),
		A: 0xff}
	vector.StrokeLine(screen, d.x, d.y, d.x+wind/2, d.y+d.size, 1, c, true)
}

func (d *Drop) minimumSize() float32 {
	return 1
}

func (d *Drop) maximumSize() float32 {
	return 6
}

func (d *Drop) calcMass() {
	d.mass = d.size * d.thickness
}

func (d *Drop) calcVelocity() {
	d.velocity = MapValue(d.mass, d.minimumSize(), d.maximumSize()*2, 7, 19)
}

func (d *Drop) regen() {
	d.size = rand.Float32()*(d.maximumSize()*3-d.minimumSize()) + d.minimumSize()
	d.stroke = MapValue(d.size, d.minimumSize(), d.maximumSize()*3, 50, 200)
	if d.stroke > 198 {
		d.thickness = 2
	} else {
		d.thickness = 1
	}

	d.calcMass()
	d.calcVelocity()
}

func MapValue(value, min1, max1, min2, max2 float32) float32 {
	return min2 + (value-min1)*(max2-min2)/(max1-min1)
}

func ResetDropPosition(d *Drop, height float32, width float32) {
	d.x = rand.Float32()*(width*2-width) + width
	d.y = rand.Float32()*(-height) + height
}
