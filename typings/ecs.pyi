import enum


def component(func):
	"""Declare the class as an ECS component"""


def system(phase: Phase, query: str = ""):
	"""Declare the function as a system"""


def spawn(*components) -> Entity:
	"""Create a new entity with specified components"""


class Entity:
	"""ECS entity"""


class Iterator:
	"""ECS query iterator"""

	def field[T](self, field_type: type[T], index: int) -> T: ...

	@property
	def count(self) -> int: ...


class Phase(enum.Enum):
	ON_LOAD = 1
	POST_LOAD = 2
	PRE_UPDATE = 3
	ON_UPDATE = 4
	ON_VALIDATE = 5
	POST_UPDATE = 6
	PRE_STORE = 7
	ON_STORE = 8
