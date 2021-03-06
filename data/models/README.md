## SMPL-X
You should have
- `data/models/smplx/SMPLX_NEUTRAL.npz`
- `data/models/smplx/SMPLX_MALE.npz`
- `data/models/smplx/SMPLX_FEMALE.npz`

1. Please go to http://smpl-x.is.tue.mpg.de and register
2. In downloads, click "download SMPL-X v...".
3. Open the downloaded zip and extract models/smplx/SMPLX_*.npz to ./smplx, so that we have: PROJ_ROOT/data/models/smplx/SMPLX_NEUTRAL.npz

## SMPL
1. Please go to http://smpl.is.tue.mpg.de and register.
2. In Downloads, download the 'for Python users' zip.
3. Unzip to project root, so that there is a folder `smpl` on the same level as CMakeLists.txt
4. `cd` to the project root directory and run `python tools/smpl2npz.py smpl/models/basicModel_f_lbs_10_207_0_v1.0.0.pkl smpl/models/basicmodel_m_lbs_10_207_0_v1.0.0.pkl`

You should have
- `data/models/smpl/SMPL_MALE.npz`
- `data/models/smpl/SMPL_FEMALE.npz`

### Neutral
1. Please go to http://smplify.is.tue.mpg.de and register.
2. Download the zip under 'Code and model'.
3. Unzip to project root, so that there is a folder `smplify_public` on the same level as CMakeLists.txt
4. `cd` to the project root directory and run `python tools/smpl2npz.py smplify_public/code/models/basicModel_neutral_lbs_10_207_0_v1.0.0.pkl`

You should have
- `data/models/smpl/SMPL_NEUTRAL.npz`

## SMPL+H
1. Please go to http://mano.is.tue.mpg.de and register
2. Download 'Extended SMPLH model for AMASS' in Downloads
3. `tar xf /path/to/smplh.tar.xz`
4. Manually copy gender/model.npz to data/models/smplh/SMPLH_GENDER.npz

You should have
- `data/models/smplh/SMPLH_NEUTRAL.npz`
- `data/models/smplh/SMPLH_MALE.npz`
- `data/models/smplh/SMPLH_FEMALE.npz`

## Notes
- SMPL+H/SMPL UV maps are not yet available (I will figure this out soon)
- SMPL-X UV map is custom-made and not optimal
