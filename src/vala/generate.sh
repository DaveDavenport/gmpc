valac -C gmpc_image.vala  --pkg gtk+-2.0 --pkg cairo
valac -C gmpc-progress2.vala  --pkg gtk+-2.0 --pkg cairo
valac -C gmpc-plugin.vala  --pkg gtk+-2.0 --pkg cairo
valac -C gmpc_rating.vala  --pkg gtk+-2.0 --pkg cairo --pkg libmpd --vapidir=`pwd`

mv gmpc-progress2.c gmpc-progress.c
mv gmpc-progress2.h gmpc-progress.h
sed -i 's/progress2.h/progress.h/g' gmpc-progress.c
