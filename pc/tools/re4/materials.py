"""Resolve editable material and group overrides without embedding source geometry."""
from dataclasses import dataclass
import math


@dataclass(frozen=True)
class SurfaceMaterial:
    category: str
    room: str
    texture_list: int
    texture: int
    uv_rect: tuple = (0., 0., 1., 1.)

    def map_uv(self, uv):
        x0,y0,x1,y1=self.uv_rect
        return (x0+uv[0]*(x1-x0),y0+uv[1]*(y1-y0))


def category(value):
    return value.category if isinstance(value,SurfaceMaterial) else value


def resolve(mapping, group, material, fallback=None):
    """Group/material > group '*' > material. Explicit null overrides must fail.

    Fallback is allowed only for unresolved base materials; callers must log it.
    Changing a class in an override selects its class donor unless a donor is supplied.
    """
    base=mapping['materials'].get(material)
    if base is None:
        raise ValueError(f'missing material mapping: {material}')
    info=dict(base)
    overrides=mapping.get('group_overrides',{}).get(group,{})
    overridden=False
    for key in ('*',material):
        if key not in overrides:continue
        override=overrides[key]
        if not isinstance(override,dict):raise ValueError('group override must be an object')
        info.update(override);overridden=True
        if 'class' in override and 'retail_texture' not in override:
            info['retail_texture']=mapping['classes'].get(override['class'])
    if info.get('exclude',False):return None,info
    cl=info['class'];ref=info.get('retail_texture')
    if ref is None and fallback is not None and not overridden:
        cl=fallback;ref=mapping['classes'].get(cl)
        info.update(provisional_fallback=cl)
    if ref is None:raise ValueError(f'unresolved donor: {group} / {material}')
    room=ref['room'];li=ref['list'];tex=ref['texture']
    if not isinstance(room,str) or len(room)!=4 or any(c not in '0123456789ABCDEFabcdef' for c in room):
        raise ValueError('donor room must be a four-digit hex ID')
    if any(type(v) is not int or v<0 for v in (li,tex)):
        raise ValueError('donor indices must be nonnegative integers')
    uv=tuple(ref.get('uv_rect',[0.,0.,1.,1.]))
    if len(uv)!=4 or not all(isinstance(v,(int,float)) and math.isfinite(v) and 0<=v<=1 for v in uv) or uv[0]>=uv[2] or uv[1]>=uv[3]:
        raise ValueError('invalid donor uv_rect')
    info.update(resolved_class=cl,resolved_reference=ref,group_override=overridden)
    return SurfaceMaterial(cl,room,li,tex,uv),info
