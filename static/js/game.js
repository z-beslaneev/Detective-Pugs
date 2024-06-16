const loadedTiles = {};
let loadedActor = undefined;
let loadedLootTypes = [];
let globalScene;
let mixers = [];
let playerId;
let camera;
let lootTypesLoaded = false;
const lootRotationSpeed = 0.0025;
const lootWaiwingSpeed = 0.005;

let pos_arr = [];

const objLoader = new THREE.OBJLoader();

function goToRecords() {
  window.location.replace('/hall_of_fame.html');
}

function loadLootType(loot, then) {
  if(loot['type'] == 'obj') {
    loot['file'] = objLoader.load(loot['file'], function(obj) {
      obj.scale.set(loot['scale'],loot['scale'],loot['scale']);
      obj.rotation.set(Math.PI*loot['rotation']/180,0,0);
      paint(obj, loot['color']);
      then(obj);
    });
  }
}

function loadLootTypes(then) {
  let objectsToLoad = map['lootTypes'].length;
  for(i in map['lootTypes']) {
    const idx = i;
    const data = map['lootTypes'][i];
    loadLootType(data, function(obj){
      loadedLootTypes[idx] = {
        obj: obj,
        data: data
      };
      if (--objectsToLoad == 0) {
        lootTypesLoaded = true;
        then();
      }
    });
  }
}

function vec_norm(v1) {
  return Math.hypot(v1[0], v1[1]);
}
function vec_dot(v1, v2) {
  return v1[0] * v2[0] + v1[1] * v2[1];
}
function vec_sub(v1, v2) {
  return [v1[0] - v2[0], v1[1] - v2[1]];
}
function vec_sum(v1, v2) {
  return [v1[0] + v2[0], v1[1] + v2[1]];
}
function vec_mul(v1, s1) {
  return [v1[0]*s1, v1[1]*s1];
}
function vec_div(v1, s1) {
  return vec_mul(v1, 1/s1);
}
function vec3_diff(v1,v2) {
  return [v1[0]-v2[0],v1[1]-v2[1],v1[2]-v2[2]];
}
function vec3_sum(v1,v2) {
  return [v1[0]+v2[0],v1[1]+v2[1],v1[2]+v2[2]];
}
function vec3_mul(v1,s2) {
  return [v1[0]*s2,v1[1]*s2,v1[2]*s2];
}

const roadEdge = new THREE.MeshPhongMaterial({color: '#AAA'});
const roadH = 0.55;

function convDirection(d) {
  const dict = {
    R:Math.PI/2,
    U:Math.PI,
    L:Math.PI*3/2,
    D:0
  };
  return dict[d];
}

//var i = 0;

class MovingObject {
  constructor(speed_coeff, init_pos) {
    this.pos = init_pos;
    this.speed_coeff = speed_coeff;
    this.target_pos = init_pos;
    //this.idx = i++;
  }

  setTarget(target_pos) {
    this.target_pos = target_pos;
  }

  doMove(time) {
    const vec = vec3_diff(this.target_pos, this.pos);
    //const vec_len = vec3_len(vec);
    //const speed = vec3_len(vec) * this.speed_coeff;
    const c_move = Math.min(this.speed_coeff * time, 1);
    this.pos = vec3_sum(this.pos, vec3_mul(vec, c_move));
    //if (this.idx==1 && c_move != 0) console.log(this.pos[2]);
  }
}

class KeyState {
  constructor() {
    this.pressedKeys = [];
  }

  keyDown(key) {
    var index = this.pressedKeys.indexOf(key);
    if (index !== -1)
      return false;

    this.pressedKeys.push(key);
    return true;
  }
  keyUp(key) {
    var index = this.pressedKeys.indexOf(key);
    if (index === -1)
      return false;

    this.pressedKeys.splice(index, 1);
    return true;
  }

  getKeyMask() {
    return this.pressedKeys.join('');
  }

  getLastKey() {
    if (this.pressedKeys.length == 0)
      return "";
    return this.pressedKeys[this.pressedKeys.length - 1];
  }
}

class GameState {
  constructor(scene) {
    let self = this;
    this.players={};
    this.scene = scene;
    this.started = false;
    this.stateLoaded = false;
    this.playersLoaded = false;
    this.updateInProgress = false;
    this.playersSuncInProgress = false;
    this.ticks = 0;
    this.posUpdateInterval = 5;
    this.playersUpdateInterval = 50;
    this.keyState = new KeyState();
    this.currentState = {players: {}};
    this.requestInstantUpdate = false;
    this.cameraPos = undefined;
    this.lostObjects = {};
    this.disappearingLoot = {};
    this.player_elems = {};

    this._updateState(function() {
      self.stateLoaded = true;
      self._startGame();
    });
    this._syncPlayers(function() {
      self.playersLoaded = true;
      self._startGame();
    });
  }

  tick() {
    this.ticks++;

    let self = this;

    if (!this.started)
      return false;

    if ((this.ticks % this.posUpdateInterval == 0 || this.requestInstantUpdate) && !this.updateInProgress) {
      this.requestInstantUpdate = false;
      this._updateState(function() {
        self._applyDesiredState();
      });
    }

    if (this.ticks % this.playersUpdateInterval == 0 && !this.playersSuncInProgress) {
      this._syncPlayers(function(){});
    }

    self._interpolateState();
    self._instantApplyState();
  }

  keyUp(key) {
    if (this.keyState.keyUp(key)) {
      this._pressKey(this.keyState.getLastKey(), function(){})
    }
  }

  keyDown(key) {
    if (this.keyState.keyDown(key)) {
      this._pressKey(this.keyState.getLastKey(), function(){})
    }
  }

  _pressKey(keys, then) {
    const self = this;
    $.post({
      url: '/api/v1/game/player/action',
      dataType: 'json',
      contentType:"application/json",
      data: JSON.stringify({
        move: keys
      }),
      beforeSend: function (xhr) {
        xhr.setRequestHeader ("Authorization", "Bearer " + Cookies.get('authToken'));
      }
    }).done(function(x){
      self.requestInstantUpdate = true;
      then();
    })
  }

  _startGame() {
    if (this.started || !this.stateLoaded || !this.playersLoaded) {
      return;
    }

    this.started = true;
    this._applyDesiredState();
    this._instantApplyState();

    if (this.desiredState.players[playerId] !== undefined) {
      const thisPlayer = this.desiredState.players[playerId];
      this.cameraPos = thisPlayer.pos;
      this.cameraUpdateTs = performance.now();
      //this._moveCamera();
      moveCameraTo(thisPlayer.pos[0], thisPlayer.pos[1]);
    }
  }

  _interpolateState() {
    const nextTime = performance.now();
    const delta = performance.now() - this.stateTime;
    const speedMul = 1/1000;
    this.stateTime = nextTime;

    Object.entries(this.desiredState['players']).forEach(([id, playerPos]) => {
      const player = this.currentState.players[id];
      player.pos = vec_sum(player.pos, vec_mul(player.speed, delta * speedMul));

      this._movePlayersLoot(player, delta);
    });

    this._processDisappearingLoot();
  }

  _syncPlayers(then) {
    this.playersSuncInProgress = true;
    let self = this;
    $.get({
      url: '/api/v1/game/players',
      dataType: 'json',
      beforeSend: function (xhr) {
        xhr.setRequestHeader ("Authorization", "Bearer " + Cookies.get('authToken'));
      }
    }).done(function(x){
      self._updatePlayersList(x);
      then();
    }).fail(function(xhr, status, err) {
      if (err!='Unauthorized') return;
      goToRecords();
    })

    this.playersSuncInProgress = false;
  }

  _updatePlayersList(ps) {
    this.updateInProgress = true;

    let self = this;
    let allPlayers=[];

    Object.entries(ps).forEach(([id, playerData]) => {
      if (self.players[id]===undefined) {
        self._addPlayer(id, playerData);
      }
      allPlayers.push(id);
    });

    let playersToDelete=[];
    for(const id in this.players) {
      if (allPlayers.indexOf(id) < 0) {
        playersToDelete.push(id);
      }
    }

    for(const id of playersToDelete) {
      self._deletePlayer(id);
    }

    this.updateInProgress = false;
  }

  _addPlayer(id, playerData) {
    let self = this;
    self.players[id] = {
      object: addActor(this.scene),
      playerData: playerData,
      data: self.currentState.players[id] !== undefined ? self.currentState.players[id].data : undefined
    };
    self._addPlayerElem(id, playerData['name']);
  }

  _deletePlayer(id) {
    let self = this;
    this.scene.remove(self.players[id].object.object);
    if (self.players[id].data !== undefined) {
      const bag = self.players[id].data.bag;
      for(var key in bag) {
        this.scene.remove(bag[key].object);
      }
    }
    delete self.players[id];
    self._delPlayerElem(id);
  }

  _movePlayerTo(id, playerPos) {
    let self = this;
    moveActor(self.players[id].object, playerPos['pos'][0], playerPos['pos'][1], playerPos['converted_dir']);
    setAnimation(self.players[id].object, playerPos['speed'][0] != 0 || playerPos['speed'][1] != 0);

    //self._moveLootForPlayer(self.players[id]);

    if(id == playerId) {
      moveCameraTo(playerPos.pos[0], playerPos.pos[1]);
    }
  }

  _moveCamera() {
    if (this.cameraPos === undefined) {
      return;
    }

    const thisPlayer = this.desiredState.players[playerId];
    const nowTime = performance.now();
    const delta = (nowTime - this.cameraUpdateTs) / 1000;

    this.cameraPos = vec_sum(this.cameraPos, vec_mul(thisPlayer.speed, delta));
    moveCameraTo(this.cameraPos[0], this.cameraPos[1]);

    this.cameraUpdateTs = nowTime;
  }

  _instantApplyState() {
    let self = this;
    Object.entries(this.currentState['players']).forEach(([id, playerPos]) => {
      if (self.players[id]===undefined) {
        return;
      }
      self._movePlayerTo(id, playerPos);
    });
    //self._moveCamera();
  }

  _createLoot(data) {
    const object = THREE.SkeletonUtils.clone(loadedLootTypes[data['type']]['obj']);
    this.scene.add(object);
    return {
      object:object,
      data:data
    };
  }

  _processLostObjects(objects) {
    let self = this;
    const nowTime = performance.now();

    Object.entries(objects).forEach(([id, data]) => {
      if (self.lostObjects[id] === undefined) {
        self.lostObjects[id] = self._createLoot(data);
      }
      const posY = roadH + 0.3 + 0.2 * Math.sin(nowTime * lootWaiwingSpeed);
      self.lostObjects[id].object.position.set(data["pos"][0] + .5, posY, data["pos"][1] + .5);
      //self.lostObjects[id].object.position.y = lootRotationSpeed * nowTime;
    });

    const objsToDelete = [];

    for(var id in self.lostObjects) {
      if (objects[id]==undefined) {
        objsToDelete.push(id);
      }
    }

    const abandonedLoot = {};
    for(var id of objsToDelete) {
      abandonedLoot[id] = self.lostObjects[id];
      delete self.lostObjects[id];
    }

    return abandonedLoot;
  }

  _updateState(then) {
    let self = this;
    $.get({
      url: '/api/v1/game/state',
      dataType: 'json',
      beforeSend: function(xhr) {
        xhr.setRequestHeader("Authorization", "Bearer " + Cookies.get('authToken'));
      }
    }).done(function(x){
      self.desiredState = x;
      self.stateTime = performance.now();
      then();
    })
  }

  _interpolateRotation(old_pos, new_pos) {
    const pi = Math.PI;
    const rot_speed = pi / 300;

    const newDir = convDirection(new_pos['dir']);
    if (old_pos['desired_dir'] != newDir) {
      new_pos['converted_dir'] = old_pos['converted_dir'];
      new_pos['base_dir'] = old_pos['converted_dir'];
      new_pos['desired_dir'] = newDir;
      new_pos['dir_time'] = performance.now();
    }
    else {
      new_pos['base_dir'] = old_pos['base_dir'];
      new_pos['desired_dir'] = old_pos['desired_dir'];
      new_pos['dir_time'] = old_pos['dir_time'];

      const delta = performance.now() - new_pos['dir_time'];
      const rot_delta = new_pos['desired_dir'] - new_pos['base_dir'];
      var rot_dir;
      if (rot_delta <= -pi || (0 <= rot_delta && rot_delta < pi)) {
        rot_dir = 1;
      }
      else {
        rot_dir = -1;
      }

      const abs_delta = Math.abs(rot_delta);
      const real_rot_delta = abs_delta >= pi ? (2*pi - abs_delta) : abs_delta;

      if (delta * rot_speed > real_rot_delta) {
        new_pos['converted_dir'] = old_pos['desired_dir'];
      }
      else {
        new_pos['converted_dir'] = old_pos['base_dir'] + 
          rot_dir * delta * rot_speed;
      }
    }
  }

  _interpolatePosition(cur_pos, new_pos, delta) {
    const speed = vec_norm(new_pos.speed);
    const dest = vec_norm(vec_sub(cur_pos.pos, new_pos.pos));

    if (speed < 0.1 || dest > 0.3) 
      return;

    const dest_t = 0.1;

    const dest_p = vec_sum(new_pos.pos, vec_mul(new_pos.speed, dest_t));

    new_pos.pos = cur_pos.pos;
    const dest_dir = vec_sub(dest_p, cur_pos.pos);
    new_pos.speed = vec_mul(dest_dir, 1/dest_t);
  }

  _setPlayerScore(id, score) {
    if (this.player_elems[id] == undefined) return;

    this.player_elems[id].score.text(score);
  }

  _addPlayerElem(id, name) {
    const obj = new Object();
    this.player_elems[id] = obj;
    obj.elem = $('<div>');
    obj.elem.text(name + ': ');
    obj.score = $('<span>');
    obj.elem.append(obj.score);

    const tab = $('#score_table');
    tab.append(obj.elem);
  }

  _delPlayerElem(id) {
    this.player_elems[id].elem.remove();
    delete this.player_elems[id];
  }

  _applyDesiredState() {
    let self = this;

    const old_players = this.currentState['players'] !== undefined ? this.currentState['players'] : {};
    const new_players = {};
    const new_update_time = performance.now();

    const abandonedLoot = self._processLostObjects(this.desiredState['lostObjects']);

    const lastP = Object.keys(this.desiredState.players)[Object.keys(this.desiredState.players).length - 1];
    const p = this.desiredState.players[lastP].pos;
    pos_arr.push([new_update_time, [p[0],p[1]]]);

    Object.entries(this.desiredState['players']).forEach(([id, playerPos]) => {
      new_players[id] = playerPos;

      const newDir = convDirection(playerPos['dir']);
      self._setPlayerScore(id, playerPos['score']);

      if (old_players[id] !== undefined) {
        self._interpolateRotation(old_players[id], new_players[id]);
        self._interpolatePosition(old_players[id], new_players[id], new_update_time - this.currentState['update_time']);
        new_players[id].data = old_players[id].data;
      }
      else {
        const newDir = convDirection(playerPos['dir']);
        new_players[id]['converted_dir'] = newDir;
        new_players[id]['desired_dir'] = newDir;
        new_players[id]['base_dir'] = newDir;
        new_players[id]['dir_time'] = performance.now();
        new_players[id].data = {
          "bag": {}
        };

        if (self.players[id]!==undefined)
          self.players[id].data = new_players[id].data;
      }
    });

    this.currentState['update_time'] = new_update_time;
    this.currentState['players'] = new_players;
    this.currentState['lostObjects'] = this.desiredState['lostObjects'];

    self._attachLootToPlayers(abandonedLoot);
  }

  _movePlayersLoot(player, delta) {
    const self = this;

    const lootSpacing = 0.4;
    const baseShiftX = -Math.sin(player.converted_dir) * lootSpacing;
    const baseShiftY = -Math.cos(player.converted_dir) * lootSpacing;
    const shiftZ = 0.2 * Math.sin(this.stateTime * lootWaiwingSpeed);

    for(var i in player.bag) {
      const elem = player.bag[i];
      const id = elem['id'];
      if (player.data.bag[id] === undefined)
        continue;

      const mover = player.data.bag[id].mover;
      mover.setTarget([player.pos[0] + 0.5 + baseShiftX*(i), 1.5 + shiftZ, player.pos[1] + 0.5 + baseShiftY*(i)]);
      mover.doMove(delta);
      player.data.bag[id].object.position.set(mover.pos[0],mover.pos[1],mover.pos[2]);
    }
  }

  _attachLootToPlayers(abandonedLoot) {
    const self = this;

    Object.entries(this.currentState['players']).forEach(([id, player]) => {
      const all_ids = new Set();
      for(var elem of player.bag) {
        var id = elem['id'];
        all_ids.add(id);
        if (player.data.bag[id] === undefined) {
          if (abandonedLoot[id] !== undefined) {
            player.data.bag[id] = abandonedLoot[id];
            delete abandonedLoot[id];
          }
          else {
            player.data.bag[id] = self._createLoot(elem);
            player.data.bag[id].object.position.set(player.pos[0] + 0.5, roadH + 0.3, player.pos[1] + 0.5);
          }
          const pos = player.data.bag[id].object.position;
          player.data.bag[id].mover = new MovingObject(0.01, [pos.x, pos.y, pos.z]);
        }
      }

      const ids_to_delete = [];
      for (var id in player.data.bag) {
        if (!all_ids.has(parseInt(id))) {
          ids_to_delete.push(id);
        }
      }
      for (var id of ids_to_delete) {
        self._dropLoot(id, player.data.bag[id]);
        delete player.data.bag[id];
      }
    });

    for(var id in abandonedLoot) {
      self._dropLoot(id, abandonedLoot[id]);
    }
  }

  _dropLoot(id, lootObj) {
    lootObj.disappearingTime = this.stateTime;
    lootObj.prevTime = lootObj.disappearingTime;
    this.disappearingLoot[id] = lootObj;
  }

  _processDisappearingLoot() {
    const max_time = 1000;
    const ids_to_delete = [];
    for(var id in this.disappearingLoot) {
      const obj = this.disappearingLoot[id];
      const time_total = this.stateTime - obj.disappearingTime;
      if(time_total >= max_time) {
        ids_to_delete.push(id);
        continue;
      }
      const time_prev = obj.prevTime - obj.disappearingTime;
      obj.object.position.y += (time_total * time_total - time_prev * time_prev)/200000;
      obj.prevTime = this.stateTime;
    }

    for(var id of ids_to_delete) {
      const obj = this.disappearingLoot[id];
      this.scene.remove(obj.object);
      delete this.disappearingLoot[id];
    }
  }
}

function makeRoadEdge(idx, val) {
  // l t r b
  const short_pts = [[0.05, 0.95], [0.05, 0.05], [0.95, 0.05], [0.95, 0.95]];
  const long_pts = [[0.05, 0.55], [0.45, 0.05], [0.95, 0.45], [0.55, 0.95]];

  const h = idx % 2 == 0 ? 0.1 : (val ? 0.1 : 0.9);
  const w = idx % 2 != 0 ? 0.1 : (val ? 0.1 : 0.9);

  const g_l = new THREE.BoxGeometry( h, roadH + 0.01, w );
  const mesh_l = new THREE.Mesh( g_l, roadEdgeMaterial );
  mesh_l.position.set( val ? short_pts[idx][0] : long_pts[idx][0], roadH/2. + 0.005, val ? short_pts[idx][1] : long_pts[idx][1] );

  return mesh_l;
}

function makeRoadTile2(tile) {
  const geometry = new THREE.BoxGeometry( 1, roadH, 1 );
  const mesh = new THREE.Mesh( geometry, roadMaterial );
  mesh.position.set( 0.5, roadH/2., 0.5 );

  const group = new THREE.Object3D();
  group.add(mesh);
  group.add(makeRoadEdge(0, tile['l']));
  group.add(makeRoadEdge(1, tile['u']));
  group.add(makeRoadEdge(2, tile['r']));
  group.add(makeRoadEdge(3, tile['d']));

  return group;
}

const roadMaterial = new THREE.MeshBasicMaterial( { color: 0x5f5f5f } );
const roadEdgeMaterial = new THREE.MeshBasicMaterial( { color: 0xe6e6e6 } );

function makeRoads(scene) {
  for(y = ymin; y<=ymax;++y) {
    for(x = xmin; x<=xmax;++x) {
      const tile = map_tiles[y-ymin][x-xmin];
      if (!tile['road']) continue;

      const mesh = makeRoadTile2(tile);
      mesh.position.set(x, 0, y);
      scene.add(mesh);
    }
  }
}

function makeBuildings(scene) {
  const buildingH = 3;
  const materials = ['#f88', '#ff8', '#f8f', '#8f8', '#88f', '#888'].map(c => new THREE.MeshPhongMaterial({color: c}));

  for(const b of map['buildings']) {
    const x = b['x'];
    const y = b['y'];
    const w = b['w'];
    const h = b['h'];
    const geo = new THREE.BoxBufferGeometry(w, buildingH , h);
    const mesh = new THREE.Mesh(geo, materials);
    mesh.position.set(x+w/2., buildingH/2., y+h/2.);
    scene.add(mesh);
  }
}

function makeOffices(scene) {
  const buildingH = 3;
  const material = new THREE.MeshPhongMaterial({color: '#afa'});
  const cone_material = new THREE.MeshPhongMaterial({color: '#faa'});

  for(const b of map['offices']) {
    const x = b['x'] + b['offsetX'];
    const y = b['y'] + b['offsetY'];
    const w = 1;
    const h = 1;
    const z = 3;
    const geo = new THREE.BoxBufferGeometry(w, z , h);
    const mesh = new THREE.Mesh(geo, material);
    mesh.position.set(x+w/2., z/2., y+h/2.);
    scene.add(mesh);

    const geometry = new THREE.ConeGeometry( .2, 1, 32 );
    //const material = new THREE.MeshBasicMaterial( {color: 0xffff00} );
    const cone = new THREE.Mesh( geometry, cone_material );
    cone.position.set(x+w/2., z/2. + 2, y+h/2.);
    cone.rotation.set(0, 0, Math.PI);
    scene.add( cone );
  }
}

function loadActor(/*, x, y*/then) {
  const fbxLoader = new THREE.FBXLoader();
  fbxLoader.load('assets/pug.fbx', (fbx) => {
      then(fbx);
  });
}

function getRandomInt(max) {
  return Math.floor(Math.random() * max);
}

function getRandomColor(max) {
  r = getRandomInt(max);
  g = getRandomInt(max);
  b = getRandomInt(max);

  return (r*256*256 + g*256 + b);
}

function addActor(scene) {
  const scale = 0.002;

  actor = new Object();
  actor.object = THREE.SkeletonUtils.clone(loadedActor);
  actor.object.visible = false;
  actor.object.scale.set(scale, scale, scale);

  animate(actor);
  paint(actor.object, getRandomColor(127)); // 0x442233

  scene.add(actor.object);
  return actor;
}

function moveCameraTo(x, y) {
  camera.position.set(x + 0, 15, y + 5);
}

function moveActor(actor, x, y, dir) {
  actor.object.visible = true;
  actor.object.position.set(x + .5, roadH, y + .5);
  actor.object.rotation.y = dir;
}

function setAnimation(actor, moving) {
  if(actor.moving == moving)
    return;

  if (moving) actor.action.play();
  else actor.action.stop();

  actor.moving = moving;
}

function isReady() {
  if (loadedActor===undefined || !lootTypesLoaded) return false;

  return true;
}

function animate(model) {
  const object = model.object;

  m = new THREE.AnimationMixer(object);
  mixers.push(m);
  model.action = m.clipAction( loadedActor.animations[ 0 ] );
  model.moving = false;
}

function paint(model, color) {
  var mat = new THREE.MeshPhongMaterial({
    color: color,
    //skinning: true ,
    //morphTargets :true,
    specular: 0x1d1c3a,
    reflectivity: 0.8,
    shininess: 20,} );

  model.traverse( function ( child ) {
    if ( child.isMesh ) {
      child.castShadow = true;
      //child.receiveShadow = true;
      child.material = mat;
    }
  });
}

function setupKeyEvents(gameState) {
  const arrowUp = 38;
  const arrowDown = 40;
  const arrowLeft = 37;
  const arrowRight = 39;
  const keyMap = {
    [arrowUp]:"U",
    [arrowRight]: "R",
    [arrowDown]: "D",
    [arrowLeft]: "L"
  };
  $(document).keydown( function(event) {
    if (keyMap[event.which] !== undefined) {
      event.preventDefault();
      gameState.keyDown(keyMap[event.which]);
    }
  }); 
  $(document).keyup( function(event) {
    if (keyMap[event.which] !== undefined) {
      event.preventDefault();
      gameState.keyUp(keyMap[event.which]);
    }
  }); 
}

function gameserverMain() {
  playerId = Cookies.get("playerId");

  const renderer = new THREE.WebGLRenderer( { antialias: true } );
  renderer.depthTest = false;

  renderer.shadowMap.enabled = true;
  renderer.setPixelRatio( window.devicePixelRatio );
  renderer.setSize( window.innerWidth, window.innerHeight );
  const canvas  = renderer.domElement;
  document.body.appendChild(  canvas );

  loadActor((object)=> {
    loadedActor = object;
  });

  loadLootTypes(function(){

  });

  const basementH = 0.5;
  const fov = 45;
  const aspect = 2;  // the canvas default
  const near = 0.1;
  const far = 2000;
  camera = new THREE.PerspectiveCamera(fov, aspect, near, far);
  camera.position.set(0, 10, 20);
  camera.rotation.set(-0.8 * Math.PI/2,0,0);

  /*const controls = new THREE.OrbitControls(camera, canvas);
  controls.target.set(0, 5, 0);
  controls.update();*/

  const scene = new THREE.Scene();
  scene.background = new THREE.Color('#DEFEFF');
  globalScene = scene;

  const clock = new THREE.Clock();

  {
    const w = xmax - xmin + 1;
    const h = ymax - ymin + 1;
    const basementGeo = new THREE.BoxBufferGeometry(w, basementH, h);
    const basementMat = new THREE.MeshPhongMaterial({color: '#8AC'});
    const mesh = new THREE.Mesh(basementGeo, basementMat);
    mesh.receiveShadow = true;
    mesh.position.set(xmin + w/2., basementH/2., ymin + h/2.);
    scene.add(mesh);
  }

  
  {
    const skyColor = 0xB1E1FF;  // light blue
    const groundColor = 0xB97A20;  // brownish orange
    const intensity = 1;
    const light = new THREE.HemisphereLight(skyColor, groundColor, intensity);
    scene.add(light);
  }

  makeRoads(scene);
  makeBuildings(scene);
  makeOffices(scene);

  {
    const color = 0xFFFFFF;
    const intensity = 1;
    const light = new THREE.DirectionalLight(color, intensity);
    light.castShadow = true;
    light.position.set(-2.50, 8.00, -8.50);
    light.target.position.set(-5.50, 0.40, -45.0);

    light.shadow.bias = -0.004;
    light.shadow.mapSize.width = 2.048;
    light.shadow.mapSize.height = 2.048;

    scene.add(light);
    scene.add(light.target);
    const cam = light.shadow.camera;
    cam.near = 0.01;
    cam.far = 20.00;
    cam.left = -15.00;
    cam.right = 15.00;
    cam.top = 15.00;
    cam.bottom = -15.00;
  }

  function resizeRendererToDisplaySize(renderer) {
    const canvas = renderer.domElement;
    const width = canvas.clientWidth;
    const height = canvas.clientHeight;
    const needResize = canvas.width !== width || canvas.height !== height;
    if (needResize) {
      renderer.setSize(width, height, false);
    }
    return needResize;
  }

  let gameState = undefined;

  function render(time) {
    const stime = time * 0.001;
    const delta = clock.getDelta();

    if (gameState===undefined && isReady()) {
      gameState = new GameState(scene);
      setupKeyEvents(gameState);
    }

    if (gameState !== undefined) {
      gameState.tick();
    }

    if (resizeRendererToDisplaySize(renderer)) {
      const canvas = renderer.domElement;
      camera.aspect = canvas.clientWidth / canvas.clientHeight;
      camera.updateProjectionMatrix();
    }

    for (m of mixers) 
      m.update(delta * 2);
    renderer.render(scene, camera);

    requestAnimationFrame(render);
  }

  requestAnimationFrame(render);
}