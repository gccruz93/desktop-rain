//go:generate goversioninfo
package main

import (
	"fmt"
	"image"
	"log"

	"github.com/ebitengine/debugui"
	"github.com/hajimehoshi/ebiten/v2"
)

type Game struct {
	debugui debugui.DebugUI
	width   float32
	height  float32
	drops   []*Drop
	wind    float32
	tick    int
}

func NewGame() *Game {
	return &Game{
		wind: 0,
	}
}

func (g *Game) Update() error {
	g.tick++
	if g.tick%ebiten.DefaultTPS == 0 && len(g.drops) < 100 {
		g.drops = append(g.drops, NewDrop())
	}

	if _, err := g.debugui.Update(func(ctx *debugui.Context) error {
		ctx.Window("Sprites", image.Rect(10, 10, 210, 110), func(layout debugui.ContainerLayout) {
			ctx.Text(fmt.Sprintf("TPS: %0.2f", ebiten.ActualTPS()))
			ctx.Text(fmt.Sprintf("FPS: %0.2f", ebiten.ActualFPS()))
			ctx.Text(fmt.Sprintf("Spawned Drops: %d", len(g.drops)))
			ctx.Text(fmt.Sprintf("Wind: %0.f", g.wind))
			// ctx.Slider(&g.sprites.num, 0, 50000, 100)
		})
		return nil
	}); err != nil {
		return err
	}

	// mouseX, _ := ebiten.CursorPosition()

	g.wind = 0
	// g.wind = MapValue(float32(mouseX), 0, g.width, -6, 6)

	for i := range len(g.drops) {
		g.drops[i].y += g.drops[i].velocity
		g.drops[i].x += g.wind
		if g.drops[i].y > g.height {
			ResetDropPosition(g.drops[i], g.height, g.width)
		}
	}
	return nil
}

func (g *Game) Draw(screen *ebiten.Image) {
	for i := range len(g.drops) {
		g.drops[i].Draw(screen, g.wind)
	}
}

func (g *Game) Layout(outsideWidth, outsideHeight int) (int, int) {
	g.height = float32(outsideHeight)
	g.width = float32(outsideWidth)
	return outsideWidth, outsideHeight
}

func main() {
	ebiten.MaximizeWindow()
	ebiten.SetWindowDecorated(false)
	ebiten.SetWindowResizingMode(ebiten.WindowResizingModeDisabled)
	ebiten.SetWindowMousePassthrough(true)
	ebiten.SetWindowTitle("Desktop Rain")

	op := &ebiten.RunGameOptions{ScreenTransparent: true}

	if err := ebiten.RunGameWithOptions(NewGame(), op); err != nil {
		log.Fatal(err)
	}
}
