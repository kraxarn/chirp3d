import math
import ecs


@ecs.component
class Position(math.Vector2):
	pass


@ecs.component
class Velocity(math.Vector2):
	pass


@ecs.system(ecs.Phase.ON_UPDATE, "Position, [in] Velocity")
def move(it: ecs.Iterator):
	pos: Position = it.get(0)
	vel: Velocity = it.get(1)
	pos = it.set(0, pos + vel)
	print("position:", pos, "velocity:", vel)


if __name__ == "__main__":
	ecs.spawn(Position(10, 20), Velocity(1, 2))
