def create_nodes_spine(femmesh):
    # nodes
    femmesh.addNode(203.2, 25.4, 0.0, 1)
    femmesh.addNode(203.2, 25.4, 25.4, 2)
    femmesh.addNode(203.2, 0.0, 0.0, 3)
    femmesh.addNode(203.2, 0.0, 25.4, 4)
    femmesh.addNode(0.0, 25.4, 0.0, 5)
    femmesh.addNode(0.0, 25.4, 25.4, 6)
    femmesh.addNode(0.0, 0.0, 0.0, 7)
    femmesh.addNode(0.0, 0.0, 25.4, 8)
    femmesh.addNode(98.3488, 25.4, 25.4, 9)
    femmesh.addNode(102.616, 0.0, 25.4, 10)
    femmesh.addNode(98.3488, 25.4, 0.0, 11)
    femmesh.addNode(102.616, 0.0, 0.0, 12)
    femmesh.addNode(0.0, 12.7, 25.4, 13)
    femmesh.addNode(49.1744, 25.4, 25.4, 14)
    femmesh.addNode(150.774, 25.4, 25.4, 15)
    femmesh.addNode(203.2, 12.7, 25.4, 16)
    femmesh.addNode(51.308, 0.0, 25.4, 17)
    femmesh.addNode(152.908, 0.0, 25.4, 18)
    femmesh.addNode(0.0, 12.7, 0.0, 19)
    femmesh.addNode(49.1744, 25.4, 0.0, 20)
    femmesh.addNode(150.774, 25.4, 0.0, 21)
    femmesh.addNode(203.2, 12.7, 0.0, 22)
    femmesh.addNode(51.308, 0.0, 0.0, 23)
    femmesh.addNode(152.908, 0.0, 0.0, 24)
    femmesh.addNode(203.2, 25.4, 12.7, 25)
    femmesh.addNode(0.0, 25.4, 12.7, 26)
    femmesh.addNode(203.2, 0.0, 12.7, 27)
    femmesh.addNode(0.0, 0.0, 12.7, 28)
    femmesh.addNode(0.0, 12.7, 12.7, 29)
    femmesh.addNode(51.308, 12.7, 25.4, 30)
    femmesh.addNode(100.482, 12.7, 25.4, 31)
    femmesh.addNode(152.908, 12.7, 25.4, 32)
    femmesh.addNode(51.308, 12.7, 0.0, 33)
    femmesh.addNode(100.482, 12.7, 0.0, 34)
    femmesh.addNode(152.908, 12.7, 0.0, 35)
    femmesh.addNode(203.2, 12.7, 12.7, 36)
    femmesh.addNode(49.1744, 25.4, 12.7, 37)
    femmesh.addNode(150.774, 25.4, 12.7, 38)
    femmesh.addNode(98.3488, 25.4, 12.7, 39)
    femmesh.addNode(51.308, 0.0, 12.7, 40)
    femmesh.addNode(152.908, 0.0, 12.7, 41)
    femmesh.addNode(102.616, 0.0, 12.7, 42)
    femmesh.addNode(51.308, 12.7, 12.7, 43)
    femmesh.addNode(100.482, 12.7, 12.7, 44)
    femmesh.addNode(152.908, 12.7, 12.7, 45)
    return True


def create_elements_spine(femmesh):
    # elements
    femmesh.addVolume([6L, 5L, 12L, 11L, 26L, 33L, 43L, 37L, 20L, 34L], 1)
    femmesh.addVolume([12L, 9L, 11L, 2L, 44L, 39L, 34L, 45L, 15L, 38L], 2)
    femmesh.addVolume([4L, 3L, 1L, 12L, 27L, 22L, 36L, 41L, 24L, 35L], 3)
    femmesh.addVolume([12L, 2L, 11L, 1L, 45L, 38L, 34L, 35L, 25L, 21L], 4)
    femmesh.addVolume([12L, 2L, 1L, 4L, 45L, 25L, 35L, 41L, 16L, 36L], 5)
    femmesh.addVolume([12L, 9L, 2L, 10L, 44L, 15L, 45L, 42L, 31L, 32L], 6)
    femmesh.addVolume([12L, 2L, 4L, 10L, 45L, 16L, 41L, 42L, 32L, 18L], 7)
    femmesh.addVolume([12L, 9L, 10L, 6L, 44L, 31L, 42L, 43L, 14L, 30L], 8)
    femmesh.addVolume([12L, 9L, 6L, 11L, 44L, 14L, 43L, 34L, 39L, 37L], 9)
    femmesh.addVolume([7L, 6L, 5L, 12L, 29L, 26L, 19L, 23L, 43L, 33L], 10)
    femmesh.addVolume([7L, 8L, 6L, 12L, 28L, 13L, 29L, 23L, 40L, 43L], 11)
    femmesh.addVolume([10L, 12L, 6L, 8L, 42L, 43L, 30L, 17L, 40L, 13L], 12)
    return True
