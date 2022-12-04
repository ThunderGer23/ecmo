from PIL import Image

imagen = Image.open("VHBA2.bmp")
print(imagen.alpha_composite)