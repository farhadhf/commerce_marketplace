<?php

/*
 * To change this template, choose Tools | Templates
 * and open the template in the editor.
 */

/**
 * Lists members of an store.
 */
function commerce_store_access_people_page($form, &$form_state, $store) {
  // Lists members in a tableselect, allows assigning/revoking roles to them, or removing them from the store.
  $form_state['store'] = $store;
  $form['operations'] = array(
    '#type' => 'fieldset',
    '#title' => t('Operations'),
    '#collapsed' => TRUE,
    '#collapsible' => TRUE,
  );
  $options = array();
  foreach (array('assign' => 'Assign @role role to selected users', 'revoke' => 'Revoke role @role from selected users') as $act => $str) {
    foreach (commerce_store_access_store_roles($store->id) as $rid => $role) {
      if (!in_array($role, array(COMMERCE_STORE_ANONYMOUS_ROLE, COMMERCE_STORE_AUTHENTICATED_ROLE))) {
        $options[$act][$act . ' ' . $rid] = t($str, array('@role' => $role));
      }
    }
  }
  $options['remove member'] = t('Remove from store');
  $form['operations']['actions'] = array(
    '#type' => 'select',
    '#options' => $options,
    '#title' => t('Action'),
    '#required' => TRUE,
  );
  $form['operations']['submit'] = array(
    '#type' => 'submit',
    '#value' => t('Go!'),
  );
  $members_field = field_get_items('commerce_store', $store, 'cmp_m_store');
  $rows = array();
  foreach ($members_field as $m) {
    $member = user_load($m['target_id']);
    $roles = commerce_store_access_get_user_roles($member, $store);
    $rows[$member->uid] = array(
      l($member->name, 'user/' . $member->uid),
      implode(', ', $roles),
    );
  }
  $header = array('Username', 'Roles');
  $form['members'] = array(
    '#type' => 'tableselect',
    '#header' => $header,
    '#options' => $rows,
  );

  return $form;
}

function commerce_store_access_people_page_submit($form, &$form_state) {
  $action = $form_state['values']['actions'];
  $store = $form_state['store'];
  if (strpos($action, 'assign') === 0) {
    if (preg_match('/assign (\d+)/', $action, $matches)) {
      $role = $matches[1];
      foreach ($form_state['values']['members'] as $uid => $selected) {
        if ($selected) {
          $member = user_load($uid);
          $member_roles = commerce_store_access_get_user_roles($member, $store);
          if (!in_array($role, array_keys($member_roles))) {
            db_insert('commerce_store_user_role')->fields(array('uid' => $member->uid, 'rid' => $role))->execute();
          }
        }
      }
    }
  }
  elseif (strpos($action, 'revoke') === 0) {
    if (preg_match('/revoke (\d+)/', $action, $matches)) {
      $role = $matches[1];
      foreach ($form_state['values']['members'] as $uid => $selected) {
        if ($selected) {
          $member = user_load($uid);
          $member_roles = commerce_store_access_get_user_roles($member, $store);
          if (in_array($role, array_keys($member_roles))) {
            db_delete('commerce_store_user_role')->condition('uid', $member->uid)->condition('rid', $role)->execute();
          }
        }
      }
    }
  }
  elseif ($action == 'remove member') {
    $members = field_get_items('commerce_store', $store, 'cmp_m_store');
    $store_roles = commerce_store_access_store_roles($store->id);
    dsm($members);
    foreach ($form_state['values']['members'] as $uid => $selected) {
      if ($selected) {
        foreach ($members as $index => $member) {
          if ($member['target_id'] == $uid) {
            unset($members[$index]);
            db_delete('commerce_store_user_role')->condition('uid', $uid)->condition('rid', array_keys($store_roles))->execute();
            break;
          }
        }
      }
    }
    $store->cmp_m_store[LANGUAGE_NONE] = $members;
    commerce_store_save($store);
  }
}

/**
 * Allows users to add new members to the store.
 */
function commerce_store_access_add_people_page($form, &$form_state, $store) {
  $form_state['store'] = $store;
  // autocomplete textfield for username.
  $form['info'] = array(
    '#type' => 'markup',
    '#markup' => t('You can add a new member to this store using this form.')
  );
  $form['user'] = array(
    '#type' => 'textfield',
    '#title' => t('User name'),
    '#autocomplete_path' => 'user/autocomplete',
  );
  $form['submit'] = array(
    '#type' => 'submit',
    '#value' => t('Add user'),
  );
  return $form;
}

/**
 * Validation function for add_people form.
 */
function commerce_store_access_add_people_page_validate($form, &$form_state) {
  $username = $form_state['values']['user'];
  $store = $form_state['store'];
  $field_members = field_get_items('commerce_store', $store, 'cmp_m_store');
  $members = array();
  foreach ($field_members as $m) {
    $members[] = $m['target_id'];
  }
  $account = user_load_by_name($username);
  if (!$account) {
    form_set_error('user', t('Invalid user name.'));
  }
  elseif (in_array($account->uid, $members)) {
    form_set_error('user', t('User @name is already a member of this store.', array('@name' => $account->name)));
  }
  else {
    $form_state['target_uid'] = $account->uid;
  }
}

/**
 * Submit function for add_people form.
 */
function commerce_store_access_add_people_page_submit($form, &$form_state) {
  $uid = $form_state['target_uid'];
  $store = $form_state['store'];
  $store->cmp_m_store[LANGUAGE_NONE][]['target_id'] = $uid;
  db_insert('commerce_store_user_role')->fields(array('uid' => $uid, 'rid' => commerce_store_access_get_store_role_id(COMMERCE_STORE_AUTHENTICATED_ROLE, $store->id)))->execute();
  commerce_store_save($store);
}

/**
 * Lists store permissions and roles in a table, like Drupal core permissions page.
 */
function commerce_store_access_perms_page($form, &$form_state, $store) {
  // Most of the following code is borrowed from og.
  $form_state['store'] = $store;
  $role_names = commerce_store_access_store_roles($store->id);

  // Fetch permissions for all roles of this store.
  $role_permissions = commerce_store_access_role_permissions($role_names);

  // Store $role_names for use when saving the data.
  $form['role_names'] = array(
    '#type' => 'value',
    '#value' => $role_names,
  );

  // Render role/permission overview:
  $options = array();
  $module_info = system_get_info('module');

  // Get a list of all the modules implementing a hook_permission() and sort by
  // display name.
  $modules = array();
  foreach (module_implements('commerce_store_access_permission') as $module) {
    $modules[$module] = $module_info[$module]['name'];
  }
  asort($modules);

  global $user;
  $status = array();
  foreach ($modules as $module => $display_name) {
    if ($permissions = module_invoke($module, 'commerce_store_access_permission')) {
      $form['permission'][] = array(
        '#markup' => $module_info[$module]['name'],
        '#id' => $module,
      );
      foreach ($permissions as $perm => $perm_item) {
        // Fill in default values for the permission.
        $perm_item += array(
          'description' => '',
          'restrict access' => FALSE,
          'warning' => !empty($perm_item['restrict access']) ? t('Warning: Give to trusted roles only; this permission has security implications in the store context.') : '',
        );
        // If the user can manage permissions, but does not have administer
        // group permission, hide restricted permissions from them. This
        // prevents users from escalating their privileges.
        if ($perm_item['restrict access'] && !commerce_store_access_user_access('change commerce_store role permissions', $store)) {
          continue;
        }

        $options[$perm] = '';
        $form['permission'][$perm] = array(
          '#type' => 'item',
          '#markup' => $perm_item['title'],
          '#description' => theme('user_permission_description', array('permission_item' => $perm_item)),
        );
        foreach ($role_names as $rid => $name) {
          // Builds arrays for checked boxes for each role
          if (isset($role_permissions[$rid][$perm])) {
            $status[$rid][] = $perm;
          }
        }
      }
    }
  }

  // Have to build checkboxes here after checkbox arrays are built
  foreach ($role_names as $rid => $name) {
    $form['checkboxes'][$rid] = array(
      '#type' => 'checkboxes',
      '#options' => $options,
      '#default_value' => isset($status[$rid]) ? $status[$rid] : array(),
      '#attributes' => array('class' => array('rid-' . $rid)),
    );
    $form['role_names'][$rid] = array('#markup' => check_plain($name), '#tree' => TRUE);
  }

  $form['actions'] = array('#type' => 'actions');
  $form['actions']['submit'] = array(
    '#type' => 'submit',
    '#value' => t('Save permissions'),
    '#submit' => array('commerce_store_access_perms_page_submit'),
  );

  return $form;
}

function commerce_store_access_perms_page_submit($form, &$form_state) {
  foreach ($form_state['values']['role_names'] as $rid => $name) {
    commerce_store_access_role_change_permissions($rid, $form_state['values'][$rid]);
  }

  drupal_set_message(t('The changes have been saved.'));
}


/**
 * Lists store roles.
 */
function commerce_store_access_roles_page($store) {
  $roles = commerce_store_access_store_roles($store->id);
  $header = array(t('Name'), array('data' => t('Operations'), 'colspan' => 2));
  $rows = array();
  foreach ($roles as $rid => $name) {
    if (!in_array($name, array(COMMERCE_STORE_ADMINISTRATOR_ROLE, COMMERCE_STORE_ANONYMOUS_ROLE, COMMERCE_STORE_AUTHENTICATED_ROLE))) {
      $rows[] = array(check_plain($name), l(t('Edit role'), 'store/' . $store->id . '/people/roles/' . $rid . '/edit'));
    }
    else {
      $rows[] = array(check_plain($name), t('Locked'));
    }
  }
  $output = theme('table', array('header' => $header, 'rows' => $rows));
  return $output;
}

/**
 * Allows users to add or edit store roles.
 */
function commerce_store_access_add_role_page($form, &$form_state, $store, $rid = FALSE) {
  $form['store_id'] = array(
    '#type' => 'value',
    '#value' => $store->id,
  );
  $role = NULL;
  if ($rid !== FALSE) {
    $role = commerce_store_access_get_role($rid);
    $form['rid'] = array(
      '#type' => 'value',
      '#value' => $rid,
    );
  }
  $form['action'] = array(
    '#type' => 'value',
    '#type' => $role ? 'edit' : 'add',
  );
  $form['name'] = array(
    '#type' => 'textfield',
    '#title' => t('Role name'),
    '#required' => TRUE,
    '#default_value' => $role ? $role->name : '',
  );
  $form['submit'] = array(
    '#type' => 'submit',
    '#value' => t('Save'),
  );
  $form['#submit'][] = 'commerce_store_role_submit';
  return $form;
}


function commerce_store_role_submit($form, &$form_state) {
  $action = $form_state['values']['action'];
  if ($action == 'edit') {
    db_update('commerce_store_role')->fields(array('rid' => $form_state['values']['rid'], 'store_id' => $form_state['values']['store_id'], 'name' => $form_state['values']['name']))->execute();
  }
  elseif ($action == 'add') {
    db_insert('commerce_store_role')->fields(array('store_id' => $form_state['values']['store_id'], 'name' => $form_state['values']['name']))->execute();
  }
}

/**
 * Theme implementation for store permissions page.
 * Code borrowed from og_ui.
 */
function theme_commerce_store_access_perms_page($variables) {
  $form = $variables['form'];
  $role_names = $form['role_names']['#value'];

  foreach (element_children($form['permission']) as $key) {
    $row = array();
    // Module name.
    if (is_numeric($key)) {
      $row[] = array('data' => drupal_render($form['permission'][$key]), 'class' => array('module'), 'id' => 'module-' . $form['permission'][$key]['#id'], 'colspan' => count($form['role_names']['#value']) + 1);
    }
    else {
      // Permission row.
      $row[] = array(
        'data' => drupal_render($form['permission'][$key]),
        'class' => array('permission'),
      );

      foreach (element_children($form['checkboxes']) as $rid) {
        $form['checkboxes'][$rid][$key]['#title'] = $role_names[$rid] . ': ' . $form['permission'][$key]['#markup'];
        $form['checkboxes'][$rid][$key]['#title_display'] = 'invisible';
        $row[] = array('data' => drupal_render($form['checkboxes'][$rid][$key]), 'class' => array('checkbox'));
      }
    }
    $rows[] = $row;
  }
  $header[] = (t('Permission'));
  foreach (element_children($form['role_names']) as $rid) {
    $header[] = array('data' => drupal_render($form['role_names'][$rid]), 'class' => array('checkbox'));
  }
  $output = '';
  $output .= theme('table', array('header' => $header, 'rows' => $rows, 'attributes' => array('id' => 'permissions')));
  $output .= drupal_render_children($form);
  return $output;
}


