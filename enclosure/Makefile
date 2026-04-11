OPENSCAD := openscad-nightly
MONTAGE := montage
targets := $(wildcard *.scad)
stls := $(targets:.scad=.stl)
image_dir := images
thumbnails := $(targets:%.scad=${image_dir}/%_thumb.png)
img_models := ${image_dir}/models.png

.PHONY: all clean clean_images images
all: ${stls}
	@echo done

${stls}: %.stl: %.scad
	@echo Building $@ from $<
	${OPENSCAD} -o $@ --backend Manifold $<

clean:
	rm -f ${stls}

clean_images:
	rm -f ${thumbnails} ${img_models}

images: $(thumbnails) ${img_models}
	@echo done

$(thumbnails): ${image_dir}/%_thumb.png: %.scad
	@echo Generating $@ from $<
	${OPENSCAD} -o $@ \
		--imgsize=480,320 --colorscheme=Tomorrow \
		--render \
		--projection o --viewall --view axes --backend Manifold $<

${img_models}: ${thumbnails}
	@echo Generating $@ from $^
	${MONTAGE} -label '%t' -pointsize 16 -geometry 480x320 $(sort $^) $@
